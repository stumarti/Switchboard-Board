// Host tests for the E1004's frame (src/board/frame.cpp): the 4-bit packing,
// every way up, the doubling and full-resolution pictures. Then every
// viewport fixture screen (viewport/test/fixtures) drawn as the E1004 shows
// it, to test/host/out/<name>.ppm as it hangs (1600x1200, or 1200x1600 for a
// portrait fixture), and the splash.
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

  // As it hangs: portrait (0, 2) 1200x1600, landscape (1, 3) 1600x1200.
  CHECK(hungW(0) == 1200 && hungH(0) == 1600 && hungW(1) == 1600 && hungH(1) == 1200);
  int nx, ny;
  // Upright: as the controllers address it.
  Frame::hungToNative(5, 7, 0, nx, ny);
  CHECK(nx == 5 && ny == 7);
  // Turned clockwise once: its bottom left corner comes to the top left,
  // its top left to the top right.
  Frame::hungToNative(0, 0, 1, nx, ny);
  CHECK(nx == 0 && ny == 1599);
  Frame::hungToNative(1599, 0, 1, nx, ny);
  CHECK(nx == 0 && ny == 0);
  // Upside down, and anticlockwise.
  Frame::hungToNative(0, 0, 2, nx, ny);
  CHECK(nx == 1199 && ny == 1599);
  Frame::hungToNative(0, 0, 3, nx, ny);
  CHECK(nx == 1199 && ny == 0);
  // Every pixel as it hangs lands on its own panel pixel, every way up.
  for (int turns = 0; turns < 4; ++turns) {
    std::vector<uint8_t> seen(NATIVE_W * NATIVE_H, 0);
    bool ok = true;
    for (int y = 0; y < hungH(turns); ++y)
      for (int x = 0; x < hungW(turns); ++x) {
        Frame::hungToNative(x, y, turns, nx, ny);
        if (nx < 0 || ny < 0 || nx >= NATIVE_W || ny >= NATIVE_H || seen[ny * NATIVE_W + nx]++) ok = false;
      }
    CHECK(ok);
  }

  // Doubled: 800x600 landscape, 600x800 portrait, each pixel 2x2, filling the panel.
  f.fill(WHITE);
  Doubled g(f, 1);
  CHECK(g.width() == 800 && g.height() == 600);
  g.drawPixel(0, 0, GxEPD_RED);
  CHECK(f.getHung(0, 0, 1) == RED && f.getHung(1, 1, 1) == RED && f.getHung(2, 0, 1) == WHITE && f.getHung(0, 2, 1) == WHITE);
  g.drawPixel(799, 599, GxEPD_BLUE);
  CHECK(f.getHung(1598, 1198, 1) == BLUE && f.getHung(1599, 1199, 1) == BLUE);
  g.drawPixel(800, 0, GxEPD_BLACK);
  g.drawPixel(0, -1, GxEPD_BLACK);
  g.fillRect(790, 590, 50, 50, GxEPD_GREEN);
  CHECK(f.getHung(1580, 1180, 1) == GREEN && f.getHung(1599, 1199, 1) == GREEN);
  Doubled p(f, 0);
  CHECK(p.width() == 600 && p.height() == 800);
  // Colours: the six inks; anything else black.
  CHECK(ink(GxEPD_WHITE) == WHITE && ink(GxEPD_BLACK) == BLACK && ink(GxEPD_RED) == RED);
  CHECK(ink(GxEPD_YELLOW) == YELLOW && ink(GxEPD_GREEN) == GREEN && ink(GxEPD_BLUE) == BLUE);
  CHECK(ink(GxEPD_ORANGE) == BLACK);

  // A picture at full resolution: a 4x2 picture placed at the screen's 10, 20
  // lands pixel for pixel at the panel's 20, 40, not doubled.
  f.fill(WHITE);
  const uint8_t pic[] = {0x12, 0x34, 0x50, 0x01};  // 1 2 3 4 / 5 0 0 1
  g.finePicture(pic, 4, 2, 10, 20);
  CHECK(f.getHung(20, 40, 1) == BLACK && f.getHung(21, 40, 1) == RED && f.getHung(22, 40, 1) == YELLOW && f.getHung(23, 40, 1) == GREEN);
  CHECK(f.getHung(20, 41, 1) == BLUE && f.getHung(21, 41, 1) == WHITE && f.getHung(23, 41, 1) == BLACK);
  CHECK(f.getHung(24, 40, 1) == WHITE && f.getHung(20, 42, 1) == WHITE);
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

// Pictures (photos) as the device keeps them: the server's spectra bitmaps,
// <FNV-1a of the src>_<w>x<h>.bin at the size asked for (twice the screen's,
// on this board), in test/fixtures/pictures here, then the viewport's.
static std::string g_picDir;
static std::map<std::string, std::string> g_pictures;
static const uint8_t* fixturePicture(const char* src, int w, int h) {
  uint32_t hash = 2166136261u;
  for (const char* p = src; *p; ++p) hash = (hash ^ static_cast<uint8_t>(*p)) * 16777619u;
  char key[64];
  snprintf(key, sizeof(key), "%08x_%dx%d", static_cast<unsigned>(hash), w, h);
  auto it = g_pictures.find(key);
  if (it == g_pictures.end()) {
    // This repository's (full-resolution) pictures, then the viewport's.
    std::string b;
    size_t from = 0;
    while (b.empty() && from <= g_picDir.size()) {
      const size_t to = g_picDir.find(':', from);
      const std::string dir = g_picDir.substr(from, to == std::string::npos ? std::string::npos : to - from);
      b = slurp((dir + "/" + key + ".bin").c_str());
      if (to == std::string::npos) break;
      from = to + 1;
    }
    if (b.size() < static_cast<size_t>((w + 1) / 2) * h) return nullptr;
    it = g_pictures.emplace(key, b).first;
  }
  return reinterpret_cast<const uint8_t*>(it->second.data());
}

static Doubled* g_drawing = nullptr;
static void finePicture(const uint8_t* bits, int w, int h, int x, int y) {
  if (g_drawing) g_drawing->finePicture(bits, w, h, x, y);
}

// The frame as it hangs, in the server preview's colours.
static void writeHung(const Frame& f, int turns, const std::string& path) {
  FILE* out = fopen(path.c_str(), "wb");
  if (!out) return;
  fprintf(out, "P6\n%d %d\n255\n", hungW(turns), hungH(turns));
  for (int y = 0; y < hungH(turns); ++y)
    for (int x = 0; x < hungW(turns); ++x) {
      uint8_t rgb[3];
      switch (f.getHung(x, y, turns)) {
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

  // render <out-dir> <icon-dir> <picture-dir> fixture.json...
  const std::string outDir = argv[1];
  g_iconDir = argv[2];
  g_picDir = argv[3];
  icons::setLookups(nullptr, fixtureIcon);
  icons::setPictureLookup(fixturePicture);
  // As display::begin() sets it up on the board.
  draw::setScreenShape(SCREEN_LONG, SCREEN_SHORT);
  draw::fine.scale = SCALE;
  draw::fine.draw = finePicture;
  std::vector<uint8_t> buf(BYTES);
  Frame f(buf.data());
  int failed = 0;
  for (int i = 4; i < argc; ++i) {
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
    // Landscape: turned clockwise once (PANEL_LANDSCAPE 1); portrait: upright.
    ctx.portrait = doc["portrait"] | false;
    const int turns = ctx.portrait ? 0 : 1;
    f.fill(WHITE);
    Doubled g(f, turns);
    g_drawing = &g;
    screens::drawScreen(g, screen, ctx);
    g_drawing = nullptr;
    std::string name = argv[i];
    name = name.substr(name.find_last_of('/') + 1);
    name = name.substr(0, name.size() - 5);
    writeHung(f, turns, outDir + "/" + name + ".ppm");
    printf("%-36s", name.c_str());
    // Pictures it would fetch (none, if they're all in the fixtures).
    screens::NullCanvas none;
    icons::beginCollect();
    screens::drawScreen(none, screen, ctx);
    icons::endCollect();
    for (int k = 0; k < icons::pictureNeedCount(); ++k) printf(" [picture %s %dx%d]", icons::pictureNeed(k).src, icons::pictureNeed(k).w, icons::pictureNeed(k).h);
    printf("\n");
  }
  // The splash, as the board shows it on power-on.
  f.fill(WHITE);
  Doubled g(f, 1);
  sys::splash(g, "starting up");
  writeHung(f, 1, outDir + "/system-splash.ppm");
  return failed ? 1 : 0;
}
