// =============================================================================
// config.h — Switchboard Board for the Seeed reTerminal E1004
//
// The viewport's config.h (viewport/src/config.h: the server, Wi-Fi setup,
// sleep and battery settings), with the E1004's pins in place of the
// E1002's. The build puts this directory first on the include path, so the
// viewport's sources get this file when they include "config.h".
//
// Pins as Seeed wired the board, and as esp32-photoframe (MIT, Wei-Ning
// Huang) drives it: components/board_hal/include/
// board_seeedstudio_reterminal_e1004.h.
// =============================================================================
#pragma once
#include_next "config.h"

#undef DEVICE_KIND
#define DEVICE_KIND "Board"

// -----------------------------------------------------------------------------
// The panel: 13.3" Spectra 6, 1200x1600, two controllers (the left half on
// EPD_CS_PIN, the right half on EPD_CS1_PIN), on a SPI bus it shares with the
// microSD card.
// -----------------------------------------------------------------------------
#undef EPD_SCK_PIN
#undef EPD_MOSI_PIN
#undef EPD_CS_PIN
#undef EPD_DC_PIN
#undef EPD_RES_PIN
#undef EPD_BUSY_PIN
#define EPD_SCK_PIN        7
#define EPD_MOSI_PIN       9
#define EPD_MISO_PIN       8    // the SD card's; the panel never answers
#define EPD_CS_PIN         10   // left half (the master controller)
#define EPD_CS1_PIN        2    // right half
#define EPD_DC_PIN         11
#define EPD_RES_PIN        38
#define EPD_BUSY_PIN       13   // LOW = busy
#define EPD_ENABLE_PIN     12   // the panel's 3V3 switch, HIGH = on

// The microSD card isn't used: it stays powered off, its chip select high.
#undef SD_EN_PIN
#define SD_EN_PIN          16   // HIGH = card powered
#define SD_CS_PIN          14

#undef LED_PIN
#define LED_PIN            48   // HIGH = on (the E1002's was inverted)

// Battery (GPIO1 through a /2 divider, switched by GPIO21) and the SHT40
// (I2C on 19/20) are where the E1002 has them.

// The three buttons, as Seeed labels them on the E1004: Refresh, right and
// left. In Switchboard's terms (the viewport's hw::Wake):
//   KEY0 home / (hold) Wi-Fi setup     the Refresh button
//   KEY1 next screen / (hold) clear    the right button
//   KEY2 previous / (hold) info        the left button
#undef BTN_KEY0
#undef BTN_KEY1
#undef BTN_KEY2
#define BTN_KEY0           5
#define BTN_KEY1           3
#define BTN_KEY2           4

// -----------------------------------------------------------------------------
// How the panel hangs. The screens are drawn 800x600 (600x800 portrait) and
// doubled to fill the 4:3 panel; the layout's rotation (Viewport layouts ->
// How it hangs) turns them. Landscape is the panel turned this many quarter
// turns clockwise from how Seeed's firmware draws it upright (portrait):
//   1  turned clockwise (its top edge on the right)
//   3  turned anticlockwise (its top edge on the left)
// If a landscape layout comes out upside down, build with the other.
// -----------------------------------------------------------------------------
#ifndef PANEL_LANDSCAPE
#define PANEL_LANDSCAPE 1
#endif
