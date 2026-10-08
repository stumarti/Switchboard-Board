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

Ink inkOfIndex(uint8_t idx) {
  static const Ink INKS[6] = {WHITE, BLACK, RED, YELLOW, GREEN, BLUE};
  return idx < 6 ? INKS[idx] : BLACK;
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

// The panel turned clockwise `turns` quarter turns: where the pixel at x, y
// as it hangs is on the panel. Turned once, the panel's bottom left corner is
// at the top left, so x runs up its left edge and y across it.
void Frame::hungToNative(int x, int y, int turns, int& nx, int& ny) {
  switch (turns & 3) {
    case 1: nx = y; ny = NATIVE_H - 1 - x; break;
    case 2: nx = NATIVE_W - 1 - x; ny = NATIVE_H - 1 - y; break;
    case 3: nx = NATIVE_W - 1 - y; ny = x; break;
    default: nx = x; ny = y; break;
  }
}

void Frame::setHung(int x, int y, Ink c, int turns) {
  if (x < 0 || y < 0 || x >= hungW(turns) || y >= hungH(turns)) return;
  int nx, ny;
  hungToNative(x, y, turns, nx, ny);
  setNative(nx, ny, c);
}

Ink Frame::getHung(int x, int y, int turns) const {
  if (x < 0 || y < 0 || x >= hungW(turns) || y >= hungH(turns)) return WHITE;
  int nx, ny;
  hungToNative(x, y, turns, nx, ny);
  return getNative(nx, ny);
}

void Native::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  const Ink c = ink(color);
  for (int yy = y; yy < y + h; ++yy)
    for (int xx = x; xx < x + w; ++xx) f_.setHung(xx, yy, c, turns_);
}

void Native::fillScreen(uint16_t color) { f_.fill(ink(color)); }

// One character, as Adafruit_GFX writes it with a custom font, but from the
// finer face at the panel's resolution: its glyph at twice the cursor, the
// cursor moved on by the screen's face's advance (so text measures and lines
// up exactly as it would doubled).
size_t Doubled::write(uint8_t c) {
  const GFXfont* fine = finer_ && gfxFont ? finer_(gfxFont) : nullptr;
  if (!fine) return Adafruit_GFX::write(c);
  if (c == '\n') {
    cursor_x = 0;
    cursor_y += textsize_y * gfxFont->yAdvance;
    return 1;
  }
  if (c == '\r' || c < gfxFont->first || c > gfxFont->last || c < fine->first || c > fine->last) return 1;
  const GFXglyph& screen = gfxFont->glyph[c - gfxFont->first];
  const GFXglyph& g = fine->glyph[c - fine->first];
  const int ts = textsize_x;
  const Ink colour = ink(textcolor);
  const int ox = cursor_x * SCALE + g.xOffset * ts, oy = cursor_y * SCALE + g.yOffset * ts;
  const uint8_t* bits = fine->bitmap + g.bitmapOffset;
  uint32_t bit = 0;
  for (int yy = 0; yy < g.height; ++yy)
    for (int xx = 0; xx < g.width; ++xx, ++bit)
      if (bits[bit >> 3] & (0x80 >> (bit & 7)))
        for (int dy = 0; dy < ts; ++dy)
          for (int dx = 0; dx < ts; ++dx) f_.setHung(ox + xx * ts + dx, oy + yy * ts + dy, colour, turns_);
  cursor_x += screen.xAdvance * ts;
  return 1;
}

void Doubled::drawPixel(int16_t x, int16_t y, uint16_t color) {
  if (x < 0 || y < 0 || x >= width() || y >= height()) return;
  const Ink c = ink(color);
  for (int dy = 0; dy < SCALE; ++dy)
    for (int dx = 0; dx < SCALE; ++dx) f_.setHung(x * SCALE + dx, y * SCALE + dy, c, turns_);
}

void Doubled::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  int x0 = x, y0 = y, x1 = x + w, y1 = y + h;
  if (w < 0) { x0 = x + w; x1 = x; }
  if (h < 0) { y0 = y + h; y1 = y; }
  if (x0 < 0) x0 = 0;
  if (y0 < 0) y0 = 0;
  if (x1 > width()) x1 = width();
  if (y1 > height()) y1 = height();
  if (x0 >= x1 || y0 >= y1) return;
  const Ink c = ink(color);
  for (int hy = y0 * SCALE; hy < y1 * SCALE; ++hy)
    for (int hx = x0 * SCALE; hx < x1 * SCALE; ++hx) f_.setHung(hx, hy, c, turns_);
}

// The whole panel.
void Doubled::fillScreen(uint16_t color) { f_.fill(ink(color)); }

void Doubled::finePicture(const uint8_t* bits, int w, int h, int x, int y) {
  const int row = (w + 1) / 2;
  const int hx = x * SCALE, hy = y * SCALE;
  for (int j = 0; j < h; ++j)
    for (int i = 0; i < w; ++i) {
      const uint8_t b = bits[j * row + i / 2];
      f_.setHung(hx + i, hy + j, inkOfIndex((i & 1) ? (b & 0x0F) : (b >> 4)), turns_);
    }
}

}  // namespace frame
