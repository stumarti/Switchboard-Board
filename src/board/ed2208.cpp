#include "board/ed2208.h"

#include <Arduino.h>
#include <SPI.h>
#include <driver/gpio.h>

#include "board/frame.h"
#include "config.h"
#include "log.h"

namespace ed2208 {

namespace {

// --- Initialisation values: Seeed's, for this panel on the E1004 ---
// (Seeed_GFX, TFT_Drivers/T133A01_Defines.h, the driver its E1004 setup
// uses.) esp32-photoframe's older set (a weaker booster, no PLL or DC-DC
// settings) left the right controller's first refresh after the panel's
// power came up washed out: its half grey and streaked, the next refresh
// fine.
const uint8_t PSR_V[] = {0xDF, 0x69};
const uint8_t PWR_V[] = {0x0F, 0x00, 0x28, 0x2C, 0x28, 0x38};
const uint8_t POF_V[] = {0x00};
const uint8_t DRF_V[] = {0x00};
const uint8_t CDI_V[] = {0x37};
const uint8_t TRES_V[] = {0x04, 0xB0, 0x03, 0x20};
const uint8_t CCSET_V[] = {0x01};
const uint8_t PWS_V[] = {0x22};
const uint8_t PLL_V[] = {0x08};
const uint8_t DCDC_V[] = {0x44, 0x54, 0x00};
const uint8_t BTST_P_V[] = {0xE0, 0x20};
const uint8_t BTST_N_V[] = {0xE0, 0x20};
const uint8_t R74[] = {0x00, 0x0C, 0x0C, 0xD9, 0xDD, 0xDD, 0x15, 0x15, 0x55};
const uint8_t RF0[] = {0x49, 0x55, 0x13, 0x5D, 0x05, 0x10};
const uint8_t R60[] = {0x03, 0x03};
const uint8_t R86[] = {0x10};
const uint8_t RB6[] = {0x07};
const uint8_t RB7[] = {0x01};
const uint8_t RB0[] = {0x01};
const uint8_t RB1[] = {0x02};
const uint8_t DSLP[] = {0xA5};

SPIClass g_spi(FSPI);
const SPISettings kSpi(10000000, MSBFIRST, SPI_MODE0);
bool g_begun = false;
// Powered and initialised (begin() since the last sleep()), and when.
bool g_awake = false;
uint32_t g_initAt = 0;
// How long after its initialisation the panel takes a picture. Sent
// straight after, the right controller sometimes isn't ready: it ignores
// the picture and refreshes its half with whatever its memory held at power
// up (noise). esp32-photoframe initialises the panel at boot and sends its
// first picture seconds later, after Wi-Fi and the download.
const uint32_t READY_MS = 1000;

// A panel pin high (1) or low (0). ESP-IDF's call, as begin() sets them up.
inline void lvl(int pin, int v) { gpio_set_level(static_cast<gpio_num_t>(pin), v); }

// A command and its data in one chip-select window, to the left controller
// (both = false) or to both.
void command(uint8_t cmd, const uint8_t* data, size_t len, bool both) {
  g_spi.beginTransaction(kSpi);
  if (both) lvl(EPD_CS1_PIN, 0);
  lvl(EPD_CS_PIN, 0);
  lvl(EPD_DC_PIN, 0);
  g_spi.transfer(cmd);
  if (len) {
    lvl(EPD_DC_PIN, 1);
    for (size_t i = 0; i < len; ++i) g_spi.transfer(data[i]);
  }
  lvl(EPD_CS_PIN, 1);
  if (both) lvl(EPD_CS1_PIN, 1);
  g_spi.endTransaction();
  if (both) delay(10);
}
void left(uint8_t cmd, const uint8_t* data = nullptr, size_t len = 0) { command(cmd, data, len, false); }
void both(uint8_t cmd, const uint8_t* data = nullptr, size_t len = 0) { command(cmd, data, len, true); }

void waitBusy(const char* what) {
  delay(10);
  const uint32_t start = millis();
  while (gpio_get_level(static_cast<gpio_num_t>(EPD_BUSY_PIN)) == 0) {
    if (millis() - start > 40000) {
      LOGF("Panel: %s still busy after 40 s\n", what);
      return;
    }
    delay(10);
  }
}

// One controller's half: 300 bytes of each of the 1600 rows.
void sendHalf(const uint8_t* fb, int pin, int colOffset) {
  constexpr int ROW = frame::NATIVE_W / 2;  // 600 bytes
  constexpr int HALF = ROW / 2;             // 300 bytes
  g_spi.beginTransaction(kSpi);
  lvl(pin, 0);
  lvl(EPD_DC_PIN, 0);
  g_spi.transfer(0x10);  // DTM
  lvl(EPD_DC_PIN, 1);
  static uint8_t line[HALF];
  for (int row = 0; row < frame::NATIVE_H; ++row) {
    memcpy(line, fb + row * ROW + colOffset, HALF);
    g_spi.writeBytes(line, HALF);
  }
  lvl(pin, 1);
  g_spi.endTransaction();
}

}  // namespace

void begin() {
  if (g_awake) return;
  // The pins a previous sleep latched low (sleep()), released and set up the
  // way esp32-photoframe does it, with ESP-IDF's own calls: the level first,
  // then the pad made a plain output (which also takes it back from the RTC
  // domain it was held in). With Arduino's digitalWrite before pinMode, a
  // wake from deep sleep could leave the right half's chip select low for
  // the first commands: the right controller took the master's settings
  // too, and refreshed its half with noise. The first refresh after power
  // on was fine; every one after a sleep wasn't.
  const int outs[] = {EPD_CS_PIN, EPD_CS1_PIN, EPD_DC_PIN, EPD_RES_PIN, EPD_ENABLE_PIN, SD_CS_PIN};
  for (int p : outs) gpio_hold_dis(static_cast<gpio_num_t>(p));
  gpio_deep_sleep_hold_dis();
  gpio_set_level(static_cast<gpio_num_t>(EPD_CS_PIN), 1);  // deselected
  gpio_set_level(static_cast<gpio_num_t>(EPD_CS1_PIN), 1);
  gpio_set_level(static_cast<gpio_num_t>(SD_CS_PIN), 1);
  gpio_set_level(static_cast<gpio_num_t>(EPD_DC_PIN), 0);
  gpio_set_level(static_cast<gpio_num_t>(EPD_RES_PIN), 1);
  gpio_set_level(static_cast<gpio_num_t>(EPD_ENABLE_PIN), 0);  // the panel off, to start clean
  gpio_config_t cs = {};
  cs.pin_bit_mask = (1ULL << EPD_CS_PIN) | (1ULL << EPD_CS1_PIN) | (1ULL << SD_CS_PIN);
  cs.mode = GPIO_MODE_OUTPUT;
  cs.pull_up_en = GPIO_PULLUP_ENABLE;
  gpio_config(&cs);
  gpio_config_t out = {};
  out.pin_bit_mask = (1ULL << EPD_DC_PIN) | (1ULL << EPD_RES_PIN) | (1ULL << EPD_ENABLE_PIN);
  out.mode = GPIO_MODE_OUTPUT;
  gpio_config(&out);
  gpio_config_t busy = {};
  busy.pin_bit_mask = 1ULL << EPD_BUSY_PIN;
  busy.mode = GPIO_MODE_INPUT;
  busy.pull_up_en = GPIO_PULLUP_ENABLE;
  gpio_config(&busy);
  delay(20);
  gpio_set_level(static_cast<gpio_num_t>(EPD_ENABLE_PIN), 1);
  delay(100);  // the panel's supply settling

  if (!g_begun) {
    g_spi.begin(EPD_SCK_PIN, EPD_MISO_PIN, EPD_MOSI_PIN, -1);
    g_begun = true;
  }

  gpio_set_level(static_cast<gpio_num_t>(EPD_RES_PIN), 0);
  delay(20);
  gpio_set_level(static_cast<gpio_num_t>(EPD_RES_PIN), 1);
  delay(20);

  waitBusy("init");
  left(0x74, R74, sizeof(R74));
  both(0xF0, RF0, sizeof(RF0));
  both(0x00, PSR_V, sizeof(PSR_V));
  both(0x30, PLL_V, sizeof(PLL_V));
  left(0xA5, DCDC_V, sizeof(DCDC_V));
  both(0x50, CDI_V, sizeof(CDI_V));
  both(0x60, R60, sizeof(R60));
  both(0x86, R86, sizeof(R86));
  both(0xE3, PWS_V, sizeof(PWS_V));
  both(0x61, TRES_V, sizeof(TRES_V));
  left(0x01, PWR_V, sizeof(PWR_V));
  delay(10);
  left(0xB6, RB6, sizeof(RB6));
  delay(10);
  left(0x06, BTST_P_V, sizeof(BTST_P_V));
  delay(10);
  left(0xB7, RB7, sizeof(RB7));
  delay(10);
  left(0x05, BTST_N_V, sizeof(BTST_N_V));
  delay(10);
  left(0xB0, RB0, sizeof(RB0));
  delay(10);
  left(0xB1, RB1, sizeof(RB1));
  delay(10);
  waitBusy("init done");
  g_awake = true;
  g_initAt = millis();
}

static uint32_t g_refreshMs = 0;

void show(const uint8_t* fb) {
  begin();
  const uint32_t since = millis() - g_initAt;
  if (since < READY_MS) delay(READY_MS - since);
  const uint32_t start = millis();
  both(0xE0, CCSET_V, sizeof(CCSET_V));
  waitBusy("ccset");
  sendHalf(fb, EPD_CS_PIN, 0);
  sendHalf(fb, EPD_CS1_PIN, frame::NATIVE_W / 4);
  both(0x04);  // PON
  waitBusy("power on");
  both(0x12, DRF_V, sizeof(DRF_V));  // refresh
  const uint32_t refreshStart = millis();
  waitBusy("refresh");
  g_refreshMs = millis() - refreshStart;
  both(0x02, POF_V, sizeof(POF_V));  // power off
  waitBusy("power off");
  LOGF("Panel refreshed in %u ms\n", static_cast<unsigned>(millis() - start));
}

uint32_t lastRefreshMs() { return g_refreshMs; }

void sleep() {
  if (!g_awake) return;
  g_awake = false;
  left(0x07, DSLP, sizeof(DSLP));  // deep sleep
  delay(1);
  waitBusy("deep sleep");
  // Panel-facing pins low before its supply goes, so they don't feed it
  // through its protection diodes; then held low through deep sleep.
  lvl(EPD_CS_PIN, 0);
  lvl(EPD_CS1_PIN, 0);
  lvl(EPD_DC_PIN, 0);
  lvl(EPD_RES_PIN, 0);
  delay(100);
  lvl(EPD_ENABLE_PIN, 0);
  for (int p : {EPD_CS_PIN, EPD_CS1_PIN, EPD_DC_PIN, EPD_RES_PIN, EPD_ENABLE_PIN}) gpio_hold_en(static_cast<gpio_num_t>(p));
  gpio_deep_sleep_hold_en();
}

}  // namespace ed2208
