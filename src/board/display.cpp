// =============================================================================
// display.cpp — the viewport's display.h on the E1004: each screen drawn
// once into the frame in PSRAM, doubled and turned landscape (frame.h), then
// sent to the panel in one refresh, and the panel put to sleep.
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
}  // namespace

void begin() {
  if (g_buf) return;
  g_buf = static_cast<uint8_t*>(ps_malloc(frame::BYTES));
  if (!g_buf) LOGF("Panel: no PSRAM for the %u-byte frame\n", static_cast<unsigned>(frame::BYTES));
}

void show(const std::function<void(draw::Gfx&)>& paint) {
  begin();
  if (!g_buf) return;
  frame::Frame f(g_buf);
  f.fill(frame::WHITE);
  frame::Doubled g(f, PANEL_LANDSCAPE);
  paint(g);
  ed2208::begin();
  ed2208::show(g_buf);
  ed2208::sleep();
}

void clear() {
  show([](draw::Gfx& g) { g.fillScreen(GxEPD_WHITE); });
}

}  // namespace display
