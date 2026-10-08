// =============================================================================
// display.cpp — the viewport's display.h on the E1004: each screen drawn
// once into the frame in PSRAM, turned as it hangs (frame.h), then sent to
// the panel in one refresh, and the panel put to sleep.
//
// A layout is shown large or small (its boardSize, given by use()):
//   large  an 800x600 screen (600x800 portrait), doubled to fill the panel,
//          with its text, icons and pictures drawn at the panel's own
//          resolution (fine.h): everything twice the size, and crisp
//   small  the panel's full 1600x1200 (1200x1600), pixel for pixel: the
//          usual sizes, four times the room
// The setup, pairing and error screens are always large.
// =============================================================================
#include "app/display.h"

#include <Arduino.h>

#include "board/ed2208.h"
#include "board/fine.h"
#include "board/frame.h"
#include "config.h"
#include "log.h"

namespace display {

namespace {
uint8_t* g_buf = nullptr;
bool g_large = true;                  // the layout about to be drawn (use())
frame::Doubled* g_drawing = nullptr;  // while a large screen is being drawn

void finePicture(const uint8_t* bits, int w, int h, int x, int y) {
  if (g_drawing) g_drawing->finePicture(bits, w, h, x, y);
}
void fineIcon(const draw::Icon& ic, int x, int y, uint16_t tint, draw::Mode mode) {
  if (g_drawing) fine::icon(*g_drawing, ic, x, y, tint, mode);
  // (Outside a draw, while measuring or collecting: nothing to put down.)
}

// How the screen is drawn: its size, and how much finer the panel is.
void configure(bool large) {
  g_large = large;
  if (large) {
    draw::setScreenShape(frame::SCREEN_LONG, frame::SCREEN_SHORT);
    draw::fine.scale = frame::SCALE;
    draw::fine.draw = finePicture;
    draw::fine.icon = fineIcon;
  } else {
    draw::setScreenShape(frame::NATIVE_H, frame::NATIVE_W);
    draw::fine.scale = 1;
    draw::fine.draw = nullptr;
    draw::fine.icon = nullptr;
  }
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
  configure(true);
  g_buf = static_cast<uint8_t*>(ps_malloc(frame::BYTES));
  if (!g_buf) LOGF("Panel: no PSRAM for the %u-byte frame\n", static_cast<unsigned>(frame::BYTES));
}

void use(int, bool large) { configure(large); }

void show(const std::function<void(draw::Gfx&)>& paint, int rotation) {
  begin();
  if (!g_buf) return;
  frame::Frame f(g_buf);
  f.fill(frame::WHITE);
  const int turns = turnsFor(rotation);
  if (g_large) {
    frame::Doubled g(f, turns, fine::finerFont);
    g_drawing = &g;
    paint(g);
    g_drawing = nullptr;
  } else {
    frame::Native g(f, turns);
    paint(g);
  }
  ed2208::begin();
  ed2208::show(g_buf);
  ed2208::sleep();
  // Back to large for whatever comes next (a setup or error screen).
  configure(true);
}

void clear() {
  show([](draw::Gfx& g) { g.fillScreen(GxEPD_WHITE); });
}

}  // namespace display
