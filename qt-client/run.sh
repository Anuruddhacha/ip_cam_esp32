#!/usr/bin/env bash
# Run the ESP32-CAM Qt desktop client. Builds first if needed.
set -euo pipefail

QT_DIR="${QT_DIR:-C:/Qt/6.11.1/mingw_64}"
MINGW_DIR="${MINGW_DIR:-C:/Qt/Tools/mingw1310_64}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Ensure Qt and MinGW runtime DLLs are findable at launch.
export PATH="$MINGW_DIR/bin:$QT_DIR/bin:$PATH"

EXE="build/esp32cam_client.exe"
if [ ! -f "$EXE" ]; then
  echo "Executable not found; building first..."
  ./build.sh
fi

exec "$EXE"
