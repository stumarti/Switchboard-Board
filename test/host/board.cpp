// Host tests for the E1004's frame (src/board/frame.cpp): the 4-bit packing,
// the landscape turn and the doubling. Then every viewport fixture screen
// (viewport/test/fixtures) drawn as the E1004 shows it, to
// test/host/out/<name>.ppm at 1600x1200 (as it hangs) and the system
// screens too.
//
//   test/host/run.sh
#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <string>
#include <vector>
#include <ArduinoJson.h>
#include "board/frame.h"
#include "render/colors.h"
#include "render/icons.h"
#include "render/screens.h"
#include "render/system.h"

using namespace frame;

static int g_fail = 0;
#define CHECK(c)                                                  \
  do {                                                            \
    if (!(c)) {                                                   \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); \
      ++g_fail;                                                   \
    }                                                             \
  } while (0)

static void testFrame() {
  std::vector<uint8_t> buf(BYTES);
  Frame f(buf.data());
  CHECK(BYTES == 960000);
  f.fill(WHITE);
  CHECK(buf[0] == 0x11 && buf[BYTES - 1] == 0x11);

  // Two pixels a byte, the left one in the high nibble.
  f.setNative(0, 0, RED);
  f.setNative(1, 0, BLUE);
  CHECK(buf[0] == 0x35);
  CHECK(f.getNative(0, 0) == RED && f.getNative(1, 0) == BLUE);
  f.setNative(1199, 1599, GREEN);
  CHECK(buf[BYTES - 1] == 0x16);
  // Off the panel: ignored.
  f.setNative(-1, 0, BLACK);
  f.setNative(1200, 0, BLACK);
  f.setNative(0, 1600, BLACK);
  CHECK(buf[0] == 0x35);

  // Landscape, turned clockwise (1): its top left is the panel's top right,
  // its top right the panel's bottom right.
  int nx, ny;
  Frame::wideToNative(0, 0, 1, nx, ny);
  CHECK(nx == 1199 && ny == 0);
  Frame::wideToNative(1599, 0, 1, nx, ny);
  CHECK(nx == 1199 && ny == 1599);
  Frame::wideToNative(0, 1199, 1, nx, ny);
  CHECK(nx == 0 && ny == 0);
  // Anticlockwise (3): the opposite corners.
  Frame::wideToNative(0, 0, 3, nx, ny);
  CHECK(nx == 0 && ny == 1599);
  Frame::wideToNative(1599, 1199, 3, nx, ny);
  CHECK(nx == 1199 && ny == 0);
  // Every landscape pixel lands on its own panel pixel.
  for (int land : {1, 3}) {
    std::vector<uint8_t> seen(NATIVE_W * NATIVE_H, 0);
    bool ok = true;
    for (int y = 0; y < WIDE_H; ++y)
      for (int x = 0; x < WIDE_W; ++x) {
        Frame::wideToNative(x, y, land, nx, ny);
        if (nx < 0 || ny < 0 || nx >= NATIVE_W || ny >= NATIVE_H || seen[ny * NATIVE_W + nx]++) ok = false;
      }
    CHECK(ok);
  }

  // Doubled: one screen pixel is 2x2, 120 px down; the margins stay white.
  f.fill(WHITE);
  Doubled g(f, 1);
  CHECK(g.width() == 800 && g.height() == 480);
  g.drawPixel(0, 0, GxEPD_RED);
  CHECK(f.getWide(0, 120, 1) == RED && f.getWide(1, 121, 1) == RED);
  CHECK(f.getWide(2, 120, 1) == WHITE && f.getWide(0, 119, 1) == WHITE && f.getWide(0, 122, 1) == WHITE);
  g.drawPixel(799, 479, GxEPD_BLUE);
  CHECK(f.getWide(1598, 1078, 1) == BLUE && f.getWide(1599, 1079, 1) == BLUE);
  CHECK(f.getWide(1599, 1080, 1) == WHITE);
  g.drawPixel(800, 0, GxEPD_BLACK);
  g.drawPixel(0, -1, GxEPD_BLACK);
  CHECK(f.getWide(0, 118, 1) == WHITE);
  // fillRect, clipped to the screen.
  g.fillRect(790, 470, 50, 50, GxEPD_GREEN);
  CHECK(f.getWide(1580, 1060, 1) == GREEN && f.getWide(1599, 1079, 1) == GREEN && f.getWide(1599, 1080, 1) == WHITE);
  // Colours: the six inks; anything else black.
  CHECK(ink(GxEPD_WHITE) == WHITE && ink(GxEPD_BLACK) == BLACK && ink(GxEPD_RED) == RED);
  CHECK(ink(GxEPD_YELLOW) == YELLOW && ink(GxEPD_GREEN) == GREEN && ink(GxEPD_BLUE) == BLUE);
  CHECK(ink(GxEPD_ORANGE) == BLACK);
}

// --- Rendering ---------------------------------------------------------------

static std::string slurp(const char* path) {
  FILE* f = fopen(path, "rb");
  if (!f) return "";
  std::string s;
  char buf[4096];
  size_t n;
  while ((n = fread(buf, 1, sizeof(buf), f)) > 0) s.append(buf, n);
  fclose(f);
  return s;
}

static std::string g_iconDir;
static std::map<std::string, std::string> g_icons;
static bool fixtureIcon(const char* name, int size, draw::Icon& out) {
  const std::string key = std::string(name) + "_" + std::to_string(size);
  auto it = g_icons.find(key);
  if (it == g_icons.end()) {
    std::string b = slurp((g_iconDir + "/" + key + ".icon").c_str());
    if (b.size() < 6) return false;
    it = g_icons.emplace(key, b).first;
  }
  const uint8_t* p = reinterpret_cast<const uint8_t*>(it->second.data());
  out.w = p[0] | (p[1] << 8);
  out.h = p[2] | (p[3] << 8);
  out.bits = p + 6;
  out.fourBit = false;
  return true;
}

// The frame as it hangs (landscape), in the server preview's colours.
static void writeWide(const Frame& f, const std::string& path) {
  FILE* out = fopen(path.c_str(), "wb");
  if (!out) return;
  fprintf(out, "P6\n%d %d\n255\n", WIDE_W, WIDE_H);
  for (int y = 0; y < WIDE_H; ++y)
    for (int x = 0; x < WIDE_W; ++x) {
      uint8_t rgb[3];
      switch (f.getWide(x, y, 1)) {
        case WHITE: rgb[0] = 255, rgb[1] = 255, rgb[2] = 255; break;
        case RED: rgb[0] = 198, rgb[1] = 40, rgb[2] = 40; break;
        case YELLOW: rgb[0] = 224, rgb[1] = 180, rgb[2] = 0; break;
        case GREEN: rgb[0] = 46, rgb[1] = 125, rgb[2] = 50; break;
        case BLUE: rgb[0] = 21, rgb[1] = 101, rgb[2] = 192; break;
        default: rgb[0] = 17, rgb[1] = 17, rgb[2] = 17; break;
      }
      fwrite(rgb, 1, 3, out);
    }
  fclose(out);
}

int main(int argc, char** argv) {
  testFrame();
  if (g_fail) {
    fprintf(stderr, "%d frame check(s) failed\n", g_fail);
    return 1;
  }
  printf("frame: all checks passed\n");
  if (argc < 3) return 0;

  // render <out-dir> <icon-dir> fixture.json...
  const std::string outDir = argv[1];
  g_iconDir = argv[2];
  icons::setLookups(nullptr, fixtureIcon);
  std::vector<uint8_t> buf(BYTES);
  Frame f(buf.data());
  int failed = 0;
  for (int i = 3; i < argc; ++i) {
    JsonDocument doc;
    if (deserializeJson(doc, slurp(argv[i]))) {
      fprintf(stderr, "%s: bad JSON\n", argv[i]);
      ++failed;
      continue;
    }
    screens::Ctx ctx;
    snprintf(ctx.time, sizeof(ctx.time), "11:04");
    ctx.battPct = 76;
    ctx.quiet = doc["quiet"] | false;
    static const char* MARKS[] = {"view-dashboard-outline", "radiator", "shield-home-outline"};
    for (int k = 0; k < 3; ++k) ctx.marks[k] = MARKS[k];
    ctx.markCount = 3;
    ctx.current = doc["current"] | 0;
    JsonObjectConst screen = doc["data"];
    f.fill(WHITE);
    Doubled g(f, 1);
    screens::drawScreen(g, screen, ctx);
    std::string name = argv[i];
    name = name.substr(name.find_last_of('/') + 1);
    name = name.substr(0, name.size() - 5);
    writeWide(f, outDir + "/" + name + ".ppm");
    printf("%s\n", name.c_str());
  }
  // The splash, as the board shows it on power-on.
  f.fill(WHITE);
  Doubled g(f, 1);
  sys::splash(g, "starting up");
  writeWide(f, outDir + "/system-splash.ppm");
  return failed ? 1 : 0;
}
