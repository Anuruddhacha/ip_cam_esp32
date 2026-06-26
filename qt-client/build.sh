#!/usr/bin/env bash
# Build the ESP32-CAM Qt desktop client (Windows + MinGW/Qt).
# Override any path by exporting the variable before running, e.g.:
#   QT_DIR="C:/Qt/6.8.0/mingw_64" ./build.sh
set -euo pipefail

QT_DIR="${QT_DIR:-C:/Qt/6.11.1/mingw_64}"
MINGW_DIR="${MINGW_DIR:-C:/Qt/Tools/mingw1310_64}"
NINJA="${NINJA:-C:/Qt/Tools/Ninja/ninja.exe}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# MinGW bin must be on PATH so the compiler finds its runtime DLLs.
export PATH="$MINGW_DIR/bin:$QT_DIR/bin:$PATH"

# Clean any stale cache (e.g. from a different generator/path) for a
# reproducible configure. Pass --keep to skip.
if [ "${1:-}" != "--keep" ]; then
  rm -rf build
fi

echo "Configuring (Qt: $QT_DIR)..."
cmake -B build -S . -G Ninja \
  -DCMAKE_MAKE_PROGRAM="$NINJA" \
  -DCMAKE_PREFIX_PATH="$QT_DIR" \
  -DCMAKE_C_COMPILER="$MINGW_DIR/bin/gcc.exe" \
  -DCMAKE_CXX_COMPILER="$MINGW_DIR/bin/g++.exe"

echo "Building..."
cmake --build build

# Bundle Qt DLLs so the .exe can run standalone (ignore if it fails).
"$QT_DIR/bin/windeployqt.exe" --release build/esp32cam_client.exe >/dev/null 2>&1 || true

echo "Done -> build/esp32cam_client.exe"
