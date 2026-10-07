#include "board/ed2208.h"

#include <Arduino.h>
#include <SPI.h>
#include <driver/gpio.h>

#include "board/frame.h"
#include "config.h"
#include "log.h"

namespace ed2208 {

namespace {

// --- Initialisation values (T133A01_Defines.h, via esp32-photoframe) ---
const uint8_t PSR_V[] = {0xDF, 0x69};
const uint8_t PWR_V[] = {0x0F, 0x00, 0x28, 0x2C, 0x28, 0x38};
const uint8_t POF_V[] = {0x00};
const uint8_t DRF_V[] = {0x01};
const uint8_t CDI_V[] = {0x37};
const uint8_t TRES_V[] = {0x04, 0xB0, 0x03, 0x20};
const uint8_t CCSET_V[] = {0x01};
const uint8_t PWS_V[] = {0x22};
const uint8_t BTST_P_V[] = {0xD8, 0x18};
const uint8_t BTST_N_V[] = {0xD8, 0x18};
const uint8_t R74[] = {0xC0, 0x1C, 0x1C, 0xCC, 0xCC, 0xCC, 0x15, 0x15, 0x55};
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

// A command and its data in one chip-select window, to the left controller
// (both = false) or to both.
void command(uint8_t cmd, const uint8_t* data, size_t len, bool both) {
  g_spi.beginTransaction(kSpi);
  if (both) digitalWrite(EPD_CS1_PIN, LOW);
  digitalWrite(EPD_CS_PIN, LOW);
  digitalWrite(EPD_DC_PIN, LOW);
  g_spi.transfer(cmd);
  if (len) {
    digitalWrite(EPD_DC_PIN, HIGH);
    for (size_t i = 0; i < len; ++i) g_spi.transfer(data[i]);
  }
  digitalWrite(EPD_CS_PIN, HIGH);
  if (both) digitalWrite(EPD_CS1_PIN, HIGH);
  g_spi.endTransaction();
  if (both) delay(10);
}
void left(uint8_t cmd, const uint8_t* data = nullptr, size_t len = 0) { command(cmd, data, len, false); }
void both(uint8_t cmd, const uint8_t* data = nullptr, size_t len = 0) { command(cmd, data, len, true); }

void waitBusy(const char* what) {
  delay(10);
  const uint32_t start = millis();
  while (digitalRead(EPD_BUSY_PIN) == LOW) {
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
  digitalWrite(pin, LOW);
  digitalWrite(EPD_DC_PIN, LOW);
  g_spi.transfer(0x10);  // DTM
  digitalWrite(EPD_DC_PIN, HIGH);
  static uint8_t line[HALF];
  for (int row = 0; row < frame::NATIVE_H; ++row) {
    memcpy(line, fb + row * ROW + colOffset, HALF);
    g_spi.writeBytes(line, HALF);
  }
  digitalWrite(pin, HIGH);
  g_spi.endTransaction();
}

}  // namespace

void begin() {
  // Pins a previous sleep latched low (sleep()).
  for (int p : {EPD_CS_PIN, EPD_CS1_PIN, EPD_DC_PIN, EPD_RES_PIN, EPD_ENABLE_PIN}) gpio_hold_dis(static_cast<gpio_num_t>(p));

  // Deselected before anything drives the bus; the SD card off.
  digitalWrite(EPD_CS_PIN, HIGH);
  digitalWrite(EPD_CS1_PIN, HIGH);
  digitalWrite(EPD_DC_PIN, LOW);
  digitalWrite(EPD_RES_PIN, HIGH);
  pinMode(EPD_CS_PIN, OUTPUT);
  pinMode(EPD_CS1_PIN, OUTPUT);
  pinMode(EPD_DC_PIN, OUTPUT);
  pinMode(EPD_RES_PIN, OUTPUT);
  pinMode(EPD_BUSY_PIN, INPUT_PULLUP);
  pinMode(SD_CS_PIN, OUTPUT);
  digitalWrite(SD_CS_PIN, HIGH);

  pinMode(EPD_ENABLE_PIN, OUTPUT);
  digitalWrite(EPD_ENABLE_PIN, HIGH);
  delay(100);  // the panel's supply settling

  if (!g_begun) {
    g_spi.begin(EPD_SCK_PIN, EPD_MISO_PIN, EPD_MOSI_PIN, -1);
    g_begun = true;
  }

  digitalWrite(EPD_RES_PIN, LOW);
  delay(20);
  digitalWrite(EPD_RES_PIN, HIGH);
  delay(20);

  waitBusy("init");
  left(0x74, R74, sizeof(R74));
  both(0xF0, RF0, sizeof(RF0));
  both(0x00, PSR_V, sizeof(PSR_V));
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
}

void show(const uint8_t* fb) {
  const uint32_t start = millis();
  both(0xE0, CCSET_V, sizeof(CCSET_V));
  waitBusy("ccset");
  sendHalf(fb, EPD_CS_PIN, 0);
  sendHalf(fb, EPD_CS1_PIN, frame::NATIVE_W / 4);
  both(0x04);  // PON
  waitBusy("power on");
  both(0x12, DRF_V, sizeof(DRF_V));  // refresh
  waitBusy("refresh");
  both(0x02, POF_V, sizeof(POF_V));  // power off
  waitBusy("power off");
  LOGF("Panel refreshed in %u ms\n", static_cast<unsigned>(millis() - start));
}

void sleep() {
  left(0x07, DSLP, sizeof(DSLP));  // deep sleep
  delay(1);
  waitBusy("deep sleep");
  // Panel-facing pins low before its supply goes, so they don't feed it
  // through its protection diodes; then held low through deep sleep.
  digitalWrite(EPD_CS_PIN, LOW);
  digitalWrite(EPD_CS1_PIN, LOW);
  digitalWrite(EPD_DC_PIN, LOW);
  digitalWrite(EPD_RES_PIN, LOW);
  delay(100);
  digitalWrite(EPD_ENABLE_PIN, LOW);
  for (int p : {EPD_CS_PIN, EPD_CS1_PIN, EPD_DC_PIN, EPD_RES_PIN, EPD_ENABLE_PIN}) gpio_hold_en(static_cast<gpio_num_t>(p));
  gpio_deep_sleep_hold_en();
}

}  // namespace ed2208
