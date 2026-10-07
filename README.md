# Switchboard Board

**Your Switchboard screens at 13.3", in colour, on battery.**

Switchboard Board turns a **Seeed reTerminal E1004** into a big Switchboard wall display. The E1004 has a 13.3" Spectra 6 colour e-ink panel (1200 × 1600), three buttons, a battery and a temperature and humidity sensor. It shows the same screens as a [Switchboard Viewport](https://github.com/stumarti/Switchboard-Viewport), at twice the size, so they read from across a lobby, a meeting room or a shop.

<p><img src="docs/images/status.png" width="800" alt="The kitchen dashboard's Status screen as the E1004 shows it: doubled to 1600 by 960, centred on the landscape panel"></p>
<sub>The kitchen dashboard's Status screen as this firmware draws it on the E1004, landscape. Drawn on a computer by the host tests (test/host), from the same code that runs on the board.</sub>

> **Status: new and untested on hardware.** It builds and its drawing is tested, but nobody has run it on an E1004 yet. The panel driver is ported from a proven one (see [Credits](#credits)); if the picture is wrong or blank, please open an issue with the serial log.

## What it does

- **Every viewport layout, doubled.** The viewport's 800 × 480 screens are drawn at 2×, 1600 × 960, centred on the panel turned landscape, with a white margin above and below. Build them in [Switchboard Server](https://github.com/stumarti/Switchboard-Server)'s layout builder, exactly as for a viewport: weather, energy, heating, calendar, meeting rooms, guest Wi-Fi, messages and the rest.
- **Everything else is the viewport's.** Wi-Fi setup with your phone, pairing, quiet hours, the carousel, battery days on the server's Home page, the temperature and humidity in Home Assistant, and updates over Wi-Fi. This repository uses the viewport's code directly (the `viewport/` submodule), so a fix to a screen there reaches both displays.
- **Months on a charge.** It sleeps between refreshes and only redraws when something changed. A full refresh of the 13.3" panel takes about 20 seconds.

Coming later: full-resolution layouts made for the big panel, portrait, and photos from Immich beside live information.

## The buttons

| Button | Press | Hold (1 s) |
|---|---|---|
| **Refresh** | The first screen, fetched afresh | Wi-Fi setup |
| **Right** | Next screen | Fill the panel white |
| **Left** | Previous screen | The button card and device details |

The setup screens come from the viewport and call the Refresh button "the green button".

## Getting started

1. **Flash it** over USB-C (below), or from a release: `switchboard-board-<version>.bin` goes to offset 0.
2. **Put it on Wi-Fi** with your phone: scan the QR code on the panel. See [Setting up a viewport](https://stumarti.github.io/Switchboard/manual/viewport/setup.html); the E1004 is the same.
3. **Approve it** in Switchboard Server under **Viewports**, and give it a layout. It shows up as a viewport whose board is `e1004`.

### Updates over Wi-Fi

Releases attach `switchboard-e1004-app-<version>.bin` and its checksum. In Switchboard Server, **Settings → Updates → Sources**, add `stumarti/Switchboard-Board`. Then its releases can be added and rolled out to your E1004 boards the same way as for viewports and remotes. A board is only ever offered builds for `e1004`.

## Building

```sh
git clone --recurse-submodules https://github.com/stumarti/Switchboard-Board.git
cd Switchboard-Board
pio run -e e1004 -t upload -t monitor
```

The serial log is on the board's UART (pins 43/44), as on the E1002.

**Which way up.** By default the panel's top edge is on the right as you look at it (`PANEL_LANDSCAPE 1`). If your screens come out upside down, build with `-DPANEL_LANDSCAPE=3`.

### Host tests

```sh
test/host/run.sh
```

Builds the frame code and the viewport's screen drawing with g++ and:
- checks the packing (two pixels a byte), the landscape turn (every pixel lands on its own panel pixel) and the doubling;
- draws every viewport fixture screen as this board shows it, into `test/host/out/` at 1600 × 1200.

CI runs these and a PlatformIO build on every push.

## How it's put together

| | |
|---|---|
| `viewport/` | [Switchboard Viewport](https://github.com/stumarti/Switchboard-Viewport) as a submodule: everything except its E1002 display and hardware files |
| `src/config.h` | The viewport's `config.h`, with the E1004's pins |
| `src/board/ed2208.cpp` | The panel driver: two controllers, each driving half of the 1200 × 1600 panel |
| `src/board/frame.cpp` | The 960 KB frame in PSRAM, and the 2× landscape canvas the screens draw on |
| `src/board/display.cpp` | The viewport's `display.h`: draw, send, refresh, sleep |
| `src/board/hw.cpp` | The viewport's `hw.h`: buttons, battery, SHT40, LED, deep sleep |

To move to a newer viewport: `cd viewport && git pull origin main`, then commit the submodule.

## Credits

The panel driver (`src/board/ed2208.cpp`) and the E1004's pin assignments are ported from [esp32-photoframe](https://github.com/aitjcize/esp32-photoframe) by Wei-Ning Huang, MIT licensed. Its initialisation values come from the panel maker's reference code. See `LICENSE`.

## Licence

MIT. See `LICENSE`.
