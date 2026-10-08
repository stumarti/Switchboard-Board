// =============================================================================
// display.cpp — the viewport's display.h on the E1004: each screen drawn
// once into the frame in PSRAM, doubled to the panel turned as it hangs
// (frame.h), pictures at the panel's full resolution, then sent to the panel
// in one refresh, and the panel put to sleep.
// =============================================================================
#include "app/display.h"

#include <Arduino.h>

#include "board/ed2208.h"
#include "board/frame.h"
#include "config.h"
#include "log.h"

namespace display {

namespace {
uint8_t* g_buf = nullptr;
frame::Doubled* g_drawing = nullptr;  // while a screen is being drawn

// draw::picture's way onto the panel at its own resolution.
void finePicture(const uint8_t* bits, int w, int h, int x, int y) {
  if (g_drawing) g_drawing->finePicture(bits, w, h, x, y);
}

// The layout's rotation (the display turned that far clockwise from its
// usual landscape) in quarter turns of the panel from how its controllers
// address it. Landscape is PANEL_LANDSCAPE (config.h) turns.
int turnsFor(int rotation) {
  const int base = PANEL_LANDSCAPE == 3 ? 3 : 1;
  return (base + ((rotation / 90) % 4 + 4) % 4) % 4;
}
}  // namespace

void begin() {
  if (g_buf) return;
  // The panel is 4:3: screens drawn 800x600 (600x800 portrait) fill it
  // doubled, and every picture comes at twice that, for full resolution.
  draw::setScreenShape(frame::SCREEN_LONG, frame::SCREEN_SHORT);
  draw::fine.scale = frame::SCALE;
  draw::fine.draw = finePicture;
  g_buf = static_cast<uint8_t*>(ps_malloc(frame::BYTES));
  if (!g_buf) LOGF("Panel: no PSRAM for the %u-byte frame\n", static_cast<unsigned>(frame::BYTES));
}

void show(const std::function<void(draw::Gfx&)>& paint, int rotation) {
  begin();
  if (!g_buf) return;
  frame::Frame f(g_buf);
  f.fill(frame::WHITE);
  frame::Doubled g(f, turnsFor(rotation));
  g_drawing = &g;
  paint(g);
  g_drawing = nullptr;
  ed2208::begin();
  ed2208::show(g_buf);
  ed2208::sleep();
}

void clear() {
  show([](draw::Gfx& g) { g.fillScreen(GxEPD_WHITE); });
}

}  // namespace display
