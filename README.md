# Switchboard Board

**Your Switchboard screens at 13.3", in colour, on battery.**

Switchboard Board turns a **Seeed reTerminal E1004** into a big Switchboard wall display. The E1004 has a 13.3" Spectra 6 colour e-ink panel (1200 × 1600), three buttons, a battery and a temperature and humidity sensor. It shows the same screens as a [Switchboard Viewport](https://github.com/stumarti/Switchboard-Viewport): at twice the size so they read from across a lobby, a meeting room or a shop, or at their usual size with four times the room. Text, icons and photos are drawn at the panel's full resolution either way.

<p><img src="docs/images/status.png" width="800" alt="The kitchen dashboard's Status screen shown large: twice the size, with crisp text and icons, filling the landscape panel"></p>
<sub>The kitchen dashboard's Status screen shown <b>large</b>: twice the size, its text and icons drawn at the panel's resolution. Drawn on a computer by the host tests (test/host), from the same code that runs on the board.</sub>

<p><img src="docs/images/status-small.png" width="800" alt="The same Status screen shown small: the usual sizes on the full 1600 by 1200, with room to spare"></p>
<sub>The same screen shown <b>small</b>: the viewport's sizes on the full 1600 × 1200.</sub>

<p><img src="docs/images/frame.png" width="800" alt="A photo from Immich filling the panel as a photo frame, with the date, weather, next event and the photo's place and date in white in the bottom left corner"></p>
<sub>A <b>photo frame</b> screen: a photo from Immich at the panel's full resolution, with a little in white over it.</sub>

> **Status: new and untested on hardware.** It builds and its drawing is tested, but nobody has run it on an E1004 yet. The panel driver is ported from a proven one (see [Credits](#credits)); if the picture is wrong or blank, please open an issue with the serial log.

## What it does

- **Every viewport layout, large or small.** Build them in [Switchboard Server](https://github.com/stumarti/Switchboard-Server)'s layout builder, exactly as for a viewport: weather, energy, heating, calendar, meeting rooms, guest Wi-Fi, messages, photos and the rest. Then choose how the layout is shown **On a 13.3” board**:
  - **Large** (the default): the screen is laid out at 800 × 600 (600 × 800 in portrait) and drawn twice the size to fill the panel. Text is drawn in fonts made at twice the size (`src/fonts`), icons are fetched at twice the size (and the built-in ones smoothly enlarged), so nothing looks blocky.
  - **Small**: the screen is laid out on the panel's whole 1600 × 1200, at the viewport's usual sizes, so it fits far more: longer lists, more of the calendar, wider columns.

  Set **Preview on** to E1004 in the builder to see the layout at the board's size.
- **A smart photo frame.** The **Photo frame** screen fills the panel with a photo from Immich and puts a little on it in white (with a black outline, so it reads on light photos, or without): the date, the weather, the next event, a message, the photo's place and date. Choose what shows, which corner and how big.
- **Photos at full resolution.** A photo frame, photo section or photo background (and album art) is fetched at the panel's own resolution and drawn pixel for pixel, up to the whole 1600 × 1200, large or small.
- **Landscape or portrait, either way up.** Set **How it hangs** on the layout: landscape, portrait turned clockwise or anticlockwise, or upside down. In portrait the screen's columns become bands, one under the other.
- **Everything else is the viewport's.** Wi-Fi setup with your phone, pairing, quiet hours, the carousel, battery days on the server's Home page, the temperature and humidity in Home Assistant, and updates over Wi-Fi. This repository uses the viewport's code directly (the `viewport/` submodule), so a fix to a screen there reaches both displays.
- **Months on a charge.** It sleeps between refreshes and only redraws when something changed. A full refresh of the 13.3" panel takes about 20 seconds.

## The buttons

| Button | Press | Hold (1 s) |
|---|---|---|
| **Refresh** | The first screen, fetched afresh | Wi-Fi setup |
| **Right** | Next screen | Fill the panel white |
| **Left** | Previous screen | The button card and device details |

The setup screens come from the viewport and call the Refresh button "the green button".

## Getting started

1. **Flash it** over USB-C from the browser with the [web flasher](https://stumarti.github.io/Switchboard/) (Chrome or Edge on desktop), which always has the latest release. Or build it yourself (below), or take `switchboard-board-<version>.bin` from a release, which goes to offset 0.
2. **Put it on Wi-Fi** with your phone: scan the QR code on the panel. See [Setting up a viewport](https://stumarti.github.io/Switchboard/manual/viewport/setup.html); the E1004 is the same.
3. **Approve it** in Switchboard Server under **Viewports**, and give it a layout. It shows up as a viewport whose board is `e1004`.

### Releasing

Push a tag (`git tag v0.1.0 && git push origin v0.1.0`). The release workflow builds the E1004 firmware and attaches the whole flash image and the over-the-air app image (with its checksum) to a GitHub release. The web flasher picks up the new release within six hours. To have it update straight away, give this repository a `SWITCHBOARD_FLASHER_TOKEN` secret: a fine-grained token for `stumarti/Switchboard` with *Contents: read and write*.

### Updates over Wi-Fi

Releases attach `switchboard-e1004-app-<version>.bin` and its checksum. In Switchboard Server, **Settings → Updates → Sources**, add `stumarti/Switchboard-Board`. Then its releases can be added and rolled out to your E1004 boards the same way as for viewports and remotes. A board is only ever offered builds for `e1004`.

## Building

```sh
git clone --recurse-submodules https://github.com/stumarti/Switchboard-Board.git
cd Switchboard-Board
pio run -e e1004 -t upload -t monitor
```

The serial log is on the board's UART (pins 43/44), as on the E1002.

**Which way up.** Set it on the layout (**How it hangs**). The firmware takes landscape to be the panel turned clockwise from how Seeed draws it upright (`PANEL_LANDSCAPE 1`). If a landscape layout comes out upside down, build with `-DPANEL_LANDSCAPE=3`, or pick **Landscape, upside down**.

### Host tests

```sh
test/host/run.sh
```

Builds the frame code and the viewport's screen drawing with g++ and:
- checks the packing (two pixels a byte), every way up (every pixel lands on its own panel pixel), the doubling, full-resolution pictures and crisp text;
- draws every viewport fixture screen large as this board shows it, and a few small too (`-small`), into `test/host/out/` as it hangs (portrait fixtures 1200 × 1600), with icons at twice the size from `test/fixtures/icons` and full-resolution photos from `test/fixtures/pictures`.

CI runs these and a PlatformIO build on every push.

## How it's put together

| | |
|---|---|
| `viewport/` | [Switchboard Viewport](https://github.com/stumarti/Switchboard-Viewport) as a submodule: everything except its E1002 display and hardware files |
| `src/config.h` | The viewport's `config.h`, with the E1004's pins |
| `src/board/ed2208.cpp` | The panel driver: two controllers, each driving half of the 1200 × 1600 panel |
| `src/board/frame.cpp` | The 960 KB frame in PSRAM, turned as it hangs; the canvases the screens draw on (doubled for large, pixel for pixel for small), crisp text and full-resolution pictures |
| `src/board/fine.cpp` | Large layouts at full resolution: which twice-the-size font for each built-in one, and icons (Scale2x for the built-in ones) |
| `src/fonts/` | Atkinson Hyperlegible at twice the viewport's sizes, and how to make them again |
| `src/board/display.cpp` | The viewport's `display.h`: large or small, draw, send, refresh, sleep |
| `src/board/hw.cpp` | The viewport's `hw.h`: buttons, battery, SHT40, LED, deep sleep |

To move to a newer viewport: `cd viewport && git pull origin main`, then commit the submodule.

## Credits

Text is set in [Atkinson Hyperlegible](https://fonts.google.com/specimen/Atkinson+Hyperlegible) by the Braille Institute, SIL Open Font License 1.1 (`src/fonts/OFL.txt`).

The panel driver (`src/board/ed2208.cpp`) and the E1004's pin assignments are ported from [esp32-photoframe](https://github.com/aitjcize/esp32-photoframe) by Wei-Ning Huang, MIT licensed. Its initialisation values come from the panel maker's reference code. See `LICENSE`.

## Licence

MIT. See `LICENSE`.
