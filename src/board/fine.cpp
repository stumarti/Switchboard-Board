#include "board/fine.h"

#include "fonts/AtkinsonHyperlegible_Bold24pt7b.h"
#include "fonts/AtkinsonHyperlegible_Bold84pt7b.h"
#include "fonts/AtkinsonHyperlegible_Regular18pt7b.h"

namespace fine {

// The viewport's faces (render/draw.h), at twice the size: REG18 and BOLD18
// are both its 9 pt regular, BOLD24 its 12 pt bold, BOLD82 its 42 pt bold.
const GFXfont* finerFont(const GFXfont* f) {
  if (!f) return nullptr;
  if (f == draw::builtInFont(draw::REG18) || f == draw::builtInFont(draw::BOLD18)) return &AtkinsonHyperlegible_Regular18pt7b;
  if (f == draw::builtInFont(draw::BOLD24)) return &AtkinsonHyperlegible_Bold24pt7b;
  if (f == draw::builtInFont(draw::BOLD82)) return &AtkinsonHyperlegible_Bold84pt7b;
  return nullptr;
}

namespace {
// An icon's pixels as values: 4-bit, its palette index (6 and up: nothing);
// 1-bit, 1 for ink and 0 for none.
uint8_t valueAt(const draw::Icon& ic, int x, int y) {
  if (ic.fourBit) {
    const uint8_t b = ic.bits[y * ((ic.w + 1) / 2) + x / 2];
    return (x & 1) ? (b & 0x0F) : (b >> 4);
  }
  return (ic.bits[y * ((ic.w + 7) / 8) + x / 8] & (0x80 >> (x & 7))) ? 0 : 1;  // bit 0 is ink
}
}  // namespace

draw::Icon scale2x(const draw::Icon& ic, std::vector<uint8_t>& buf) {
  const int w = ic.w, h = ic.h, W = w * 2, H = h * 2;
  std::vector<uint8_t> v(W * H);
  auto at = [&](int x, int y) { return valueAt(ic, x < 0 ? 0 : x >= w ? w - 1 : x, y < 0 ? 0 : y >= h ? h - 1 : y); };
  for (int y = 0; y < h; ++y)
    for (int x = 0; x < w; ++x) {
      const uint8_t P = at(x, y), A = at(x, y - 1), B = at(x + 1, y), C = at(x - 1, y), D = at(x, y + 1);
      uint8_t e0 = P, e1 = P, e2 = P, e3 = P;
      if (C == A && C != D && A != B) e0 = A;
      if (A == B && A != C && B != D) e1 = B;
      if (D == C && D != B && C != A) e2 = C;
      if (B == D && B != A && D != C) e3 = D;
      v[(2 * y) * W + 2 * x] = e0;
      v[(2 * y) * W + 2 * x + 1] = e1;
      v[(2 * y + 1) * W + 2 * x] = e2;
      v[(2 * y + 1) * W + 2 * x + 1] = e3;
    }
  draw::Icon out;
  out.w = W;
  out.h = H;
  out.fourBit = ic.fourBit;
  out.fine = true;
  if (ic.fourBit) {
    const int row = (W + 1) / 2;
    buf.assign(row * H, 0);
    for (int y = 0; y < H; ++y)
      for (int x = 0; x < W; ++x) buf[y * row + x / 2] |= (x & 1) ? (v[y * W + x] & 0x0F) : (v[y * W + x] << 4);
  } else {
    const int row = (W + 7) / 8;
    buf.assign(row * H, 0xFF);
    for (int y = 0; y < H; ++y)
      for (int x = 0; x < W; ++x)
        if (v[y * W + x]) buf[y * row + x / 8] &= ~(0x80 >> (x & 7));
  }
  out.bits = buf.data();
  return out;
}

void icon(frame::Doubled& screen, const draw::Icon& ic, int x, int y, uint16_t tint, draw::Mode mode) {
  frame::Native native(screen.frame(), screen.turns());
  if (ic.fine) {
    draw::iconPixels(native, ic, x * frame::SCALE, y * frame::SCALE, tint, mode);
    return;
  }
  std::vector<uint8_t> buf;
  draw::iconPixels(native, scale2x(ic, buf), x * frame::SCALE, y * frame::SCALE, tint, mode);
}

}  // namespace fine
