#!/usr/bin/env bash
# Host tests: the E1004's frame (packing, the landscape turn, the doubling),
# then every viewport fixture screen drawn as this board shows it, built
# with g++ and run here — no ESP32, no PlatformIO.
#
#   git submodule update --init
#   test/host/run.sh
#
# Writes test/host/out/*.png as the panel hangs (1600x1200 or 1200x1600) (PNG needs Python's Pillow;
# otherwise the .ppm files are left).
set -euo pipefail
cd "$(dirname "$0")/../.."
VP=viewport
BUILD=test/host/build
OUT=test/host/out
mkdir -p "$BUILD" "$OUT"

# ArduinoJson is a single header; fetch the pinned release once.
AJ_VERSION=7.4.2
AJ="$BUILD/ArduinoJson.h"
if [ ! -f "$AJ" ]; then
  curl -sSfL -o "$AJ" "https://github.com/bblanchon/ArduinoJson/releases/download/v${AJ_VERSION}/ArduinoJson-v${AJ_VERSION}.h"
fi

CXX=${CXX:-g++}
# This repository's headers first (its config.h includes the viewport's), and
# the viewport's host stubs for Arduino and Adafruit_GFX.
FLAGS=(-std=gnu++17 -O2 -g -DSB_HOST -DARDUINOJSON_DEFAULT_NESTING_LIMIT=32 -Wall -Wno-unused-function -Wno-sign-compare
  -Isrc -I$VP/test/host/stubs -I$VP/test/host -I$VP/src -I$VP/lib/qrcodegen -I"$BUILD")
SRC=(test/host/board.cpp src/board/frame.cpp
  $VP/src/render/draw.cpp $VP/src/render/icons.cpp $VP/src/render/screens.cpp $VP/src/render/system.cpp
  $VP/lib/qrcodegen/qrcodegen.c $VP/test/host/stubs/Adafruit_GFX.cpp)

"$CXX" "${FLAGS[@]}" -o "$BUILD/board" "${SRC[@]}"
"$BUILD/board" "$OUT" $VP/test/fixtures/icons test/fixtures/pictures:$VP/test/fixtures/pictures $VP/test/fixtures/*.json

if python3 -c "import PIL" 2>/dev/null; then
  python3 - "$OUT" <<'PY'
import glob, os, sys
from PIL import Image
for p in glob.glob(os.path.join(sys.argv[1], '*.ppm')):
    Image.open(p).save(p[:-4] + '.png')
    os.remove(p)
PY
fi
echo "Rendered into $OUT"
