// =============================================================================
// frame.h — the E1004's picture in memory, and the viewport's screens on it.
//
// The panel is 1200x1600 (portrait, as the controllers address it), 4 bits a
// pixel, two pixels a byte, left pixel in the high nibble: 960,000 bytes,
// kept in PSRAM. Each nibble is the controllers' own colour code.
//
// The viewport's screens are drawn for 800x480. Doubled is the GFX they draw
// on here: each of their pixels becomes 2x2 on the panel turned landscape
// (1600x1200), centred, so they fill 1600x960 with 120 px of white above and
// below. No Arduino here, so the host tests build it too.
// =============================================================================
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <Adafruit_GFX.h>

namespace frame {

constexpr int NATIVE_W = 1200;  // as the controllers address it
constexpr int NATIVE_H = 1600;
constexpr size_t BYTES = NATIVE_W / 2 * NATIVE_H;

// Landscape, as it hangs.
constexpr int WIDE_W = NATIVE_H;  // 1600
constexpr int WIDE_H = NATIVE_W;  // 1200
constexpr int SCALE = 2;
constexpr int SRC_W = 800, SRC_H = 480;  // the viewport's screens
constexpr int OFFSET_X = (WIDE_W - SRC_W * SCALE) / 2;  // 0
constexpr int OFFSET_Y = (WIDE_H - SRC_H * SCALE) / 2;  // 120

// The controllers' colour codes.
enum Ink : uint8_t { BLACK = 0x0, WHITE = 0x1, YELLOW = 0x2, RED = 0x3, BLUE = 0x5, GREEN = 0x6 };

// A GxEPD2 colour (what the screens draw with) as the panel's ink; anything
// else (orange, a stray RGB565) as black.
Ink ink(uint16_t gxColor);

// The picture. `buf` is BYTES long (the caller owns it).
class Frame {
 public:
  explicit Frame(uint8_t* buf) : buf_(buf) {}
  uint8_t* data() { return buf_; }
  const uint8_t* data() const { return buf_; }
  void fill(Ink c);
  // A pixel in the controllers' own coordinates.
  void setNative(int x, int y, Ink c);
  Ink getNative(int x, int y) const;
  // A pixel on the panel hung landscape (1600x1200), turned as `landscape`
  // says (PANEL_LANDSCAPE: 1 or 3).
  void setWide(int x, int y, Ink c, int landscape);
  Ink getWide(int x, int y, int landscape) const;
  // Where a landscape pixel is in the controllers' coordinates.
  static void wideToNative(int x, int y, int landscape, int& nx, int& ny);

 private:
  uint8_t* buf_;
};

// The 800x480 GFX the viewport's screens draw on: every pixel 2x2, centred.
class Doubled : public Adafruit_GFX {
 public:
  Doubled(Frame& f, int landscape) : Adafruit_GFX(SRC_W, SRC_H), f_(f), landscape_(landscape) {}
  void drawPixel(int16_t x, int16_t y, uint16_t color) override;
  // Faster than one pixel at a time (fillScreen, the screens' bars). Virtual
  // in Adafruit_GFX, so these override on the device; the host tests' GFX
  // stub has them plain, and draws through drawPixel instead.
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
  void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) { fillRect(x, y, w, 1, color); }
  void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) { fillRect(x, y, 1, h, color); }
  void fillScreen(uint16_t color);

 private:
  Frame& f_;
  int landscape_;
};

}  // namespace frame
