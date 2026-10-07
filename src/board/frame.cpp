#include "board/frame.h"

#include <string.h>

#include "render/colors.h"

namespace frame {

Ink ink(uint16_t c) {
  switch (c) {
    case GxEPD_WHITE: return WHITE;
    case GxEPD_BLACK: return BLACK;
    case GxEPD_RED: return RED;
    case GxEPD_YELLOW: return YELLOW;
    case GxEPD_GREEN: return GREEN;
    case GxEPD_BLUE: return BLUE;
    default: return BLACK;
  }
}

void Frame::fill(Ink c) { memset(buf_, (c << 4) | c, BYTES); }

void Frame::setNative(int x, int y, Ink c) {
  if (x < 0 || y < 0 || x >= NATIVE_W || y >= NATIVE_H) return;
  uint8_t& b = buf_[y * (NATIVE_W / 2) + x / 2];
  b = (x & 1) ? (b & 0xF0) | c : (b & 0x0F) | (c << 4);
}

Ink Frame::getNative(int x, int y) const {
  if (x < 0 || y < 0 || x >= NATIVE_W || y >= NATIVE_H) return WHITE;
  const uint8_t b = buf_[y * (NATIVE_W / 2) + x / 2];
  return static_cast<Ink>((x & 1) ? (b & 0x0F) : (b >> 4));
}

// Turned clockwise (1), the panel's top edge is on the right: landscape x
// runs down the panel, landscape y runs right to left across it. Turned
// anticlockwise (3), the other way round.
void Frame::wideToNative(int x, int y, int landscape, int& nx, int& ny) {
  if (landscape == 3) {
    nx = y;
    ny = NATIVE_H - 1 - x;
  } else {
    nx = NATIVE_W - 1 - y;
    ny = x;
  }
}

void Frame::setWide(int x, int y, Ink c, int landscape) {
  if (x < 0 || y < 0 || x >= WIDE_W || y >= WIDE_H) return;
  int nx, ny;
  wideToNative(x, y, landscape, nx, ny);
  setNative(nx, ny, c);
}

Ink Frame::getWide(int x, int y, int landscape) const {
  if (x < 0 || y < 0 || x >= WIDE_W || y >= WIDE_H) return WHITE;
  int nx, ny;
  wideToNative(x, y, landscape, nx, ny);
  return getNative(nx, ny);
}

void Doubled::drawPixel(int16_t x, int16_t y, uint16_t color) {
  if (x < 0 || y < 0 || x >= SRC_W || y >= SRC_H) return;
  const Ink c = ink(color);
  const int wx = OFFSET_X + x * SCALE, wy = OFFSET_Y + y * SCALE;
  for (int dy = 0; dy < SCALE; ++dy)
    for (int dx = 0; dx < SCALE; ++dx) f_.setWide(wx + dx, wy + dy, c, landscape_);
}

void Doubled::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  int x0 = x, y0 = y, x1 = x + w, y1 = y + h;
  if (w < 0) { x0 = x + w; x1 = x; }
  if (h < 0) { y0 = y + h; y1 = y; }
  if (x0 < 0) x0 = 0;
  if (y0 < 0) y0 = 0;
  if (x1 > SRC_W) x1 = SRC_W;
  if (y1 > SRC_H) y1 = SRC_H;
  if (x0 >= x1 || y0 >= y1) return;
  const Ink c = ink(color);
  for (int wy = OFFSET_Y + y0 * SCALE; wy < OFFSET_Y + y1 * SCALE; ++wy)
    for (int wx = OFFSET_X + x0 * SCALE; wx < OFFSET_X + x1 * SCALE; ++wx) f_.setWide(wx, wy, c, landscape_);
}

// The whole panel, margins included.
void Doubled::fillScreen(uint16_t color) { f_.fill(ink(color)); }

}  // namespace frame
