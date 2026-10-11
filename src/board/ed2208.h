// =============================================================================
// ed2208.h — the E1004's 13.3" Spectra 6 panel (E Ink ED2208-NCA), 1200x1600.
//
// Two controllers, each driving half the panel (600 columns): CS selects the
// left one, CS1 the right one. Some commands go to both, some to the left one
// only. A refresh takes about 20 s.
//
// Ported from esp32-photoframe's driver (MIT, Wei-Ning Huang):
// components/epaper_driver_ed2208_nca/src/driver_ed2208_nca.c, whose
// initialisation values come from the panel maker's T133A01_Defines.h. Same
// commands, values and order; Arduino's SPI and GPIO in place of ESP-IDF's.
// =============================================================================
#pragma once
#include <stdint.h>

namespace ed2208 {

// Power the panel on, reset it and send its initialisation.
void begin();
// Send a whole frame (frame::BYTES, the controllers' colour codes, 2 pixels
// a byte) and refresh: returns when the panel has finished.
void show(const uint8_t* frame);
// How long the last show()'s refresh took, in ms.
uint32_t lastRefreshMs();
// Put the controllers to sleep and switch the panel's power off; the pins
// are held low through deep sleep. begin() undoes it.
void sleep();

}  // namespace ed2208
