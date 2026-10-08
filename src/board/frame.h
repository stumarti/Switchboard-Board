// =============================================================================
// frame.h — the E1004's picture in memory, and the viewport's screens on it.
//
// The panel is 1200x1600 (portrait, as the controllers address it), 4 bits a
// pixel, two pixels a byte, left pixel in the high nibble: 960,000 bytes,
// kept in PSRAM. Each nibble is the controllers' own colour code.
//
// However it hangs, the panel is 4:3, so the viewport's screens are drawn
// 800x600 (600x800 portrait: draw::setScreenShape) and doubled to fill it:
// Doubled is the GFX they draw on, each of their pixels 2x2 on the panel.
// Pictures (photos, album art) are fetched at twice the size and put on the
// panel pixel for pixel (Doubled::finePicture), at its full resolution.
//
// "Turns": how far the panel is turned clockwise from how the controllers
// address it, in quarter turns. 0 is portrait the way Seeed's own firmware
// draws it; 1 and 3 are landscape; 2 is portrait upside down.
// No Arduino here, so the host tests build it too.
// =============================================================================
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <Adafruit_GFX.h>

namespace frame {

constexpr int NATIVE_W = 1200;  // as the controllers address it
constexpr int NATIVE_H = 1600;
constexpr size_t BYTES = NATIVE_W / 2 * NATIVE_H;
constexpr int SCALE = 2;
// The screen the viewport's code draws, landscape (portrait: swapped).
constexpr int SCREEN_LONG = NATIVE_H / SCALE;   // 800
constexpr int SCREEN_SHORT = NATIVE_W / SCALE;  // 600

// The controllers' colour codes.
enum Ink : uint8_t { BLACK = 0x0, WHITE = 0x1, YELLOW = 0x2, RED = 0x3, BLUE = 0x5, GREEN = 0x6 };

// A GxEPD2 colour (what the screens draw with) as the panel's ink; anything
// else (orange, a stray RGB565) as black.
Ink ink(uint16_t gxColor);
// A picture's palette index (the server's: 0 white, 1 black, 2 red,
// 3 yellow, 4 green, 5 blue) as the panel's ink.
Ink inkOfIndex(uint8_t idx);

// Turned `turns` quarter turns: is it landscape, and how big as it hangs.
inline bool landscape(int turns) { return turns & 1; }
inline int hungW(int turns) { return landscape(turns) ? NATIVE_H : NATIVE_W; }
inline int hungH(int turns) { return landscape(turns) ? NATIVE_W : NATIVE_H; }

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
  // A pixel as the panel hangs (hungW x hungH), turned `turns`.
  void setHung(int x, int y, Ink c, int turns);
  Ink getHung(int x, int y, int turns) const;
  // Where a pixel as it hangs is in the controllers' coordinates.
  static void hungToNative(int x, int y, int turns, int& nx, int& ny);

 private:
  uint8_t* buf_;
};

// The GFX the viewport's screens draw on: 800x600 (landscape) or 600x800,
// every pixel 2x2 on the panel turned `turns`.
class Doubled : public Adafruit_GFX {
 public:
  Doubled(Frame& f, int turns)
      : Adafruit_GFX(landscape(turns) ? SCREEN_LONG : SCREEN_SHORT, landscape(turns) ? SCREEN_SHORT : SCREEN_LONG), f_(f), turns_(turns) {}
  void drawPixel(int16_t x, int16_t y, uint16_t color) override;
  // Faster than one pixel at a time (fillScreen, the screens' bars). Virtual
  // in Adafruit_GFX, so these override on the device; the host tests' GFX
  // stub has them plain, and draws through drawPixel instead.
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
  void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) { fillRect(x, y, w, 1, color); }
  void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) { fillRect(x, y, 1, h, color); }
  void fillScreen(uint16_t color);
  // A picture at the panel's resolution: `bits` w x h (4-bit palette
  // indices, rows padded to whole bytes), its top left at the screen's x, y.
  void finePicture(const uint8_t* bits, int w, int h, int x, int y);

 private:
  Frame& f_;
  int turns_;
};

}  // namespace frame
