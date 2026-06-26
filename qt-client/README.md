# ESP32-CAM Qt Desktop Viewer

A small Qt (C++) desktop client for the relay. It connects to the relay's
`/view` WebSocket endpoint and displays the live JPEG stream.

```
Relay (wss://.../view) ──binary JPEG frames──► Qt app (QWebSocket → QImage → QLabel)
```

## Requirements

- **Qt 6** (recommended) or **Qt 5.15+**, with the **WebSockets** and **Widgets** modules.
- **CMake 3.16+** and a C++17 compiler.
- TLS support (for `wss://`). Qt's official installer includes this; on Linux
  make sure OpenSSL is available.

Install Qt via the [online installer](https://www.qt.io/download-qt-installer)
and tick the "Qt WebSockets" module for your kit.

## Quick build (Windows + MinGW)

From **Git Bash** (not WSL):

```bash
cd qt-client
./build.sh      # configure + build + bundle Qt DLLs
./run.sh        # launch (builds first if needed)
```

Paths default to `C:/Qt/6.11.1/mingw_64` and the bundled MinGW/Ninja. Override
if your install differs:

```bash
QT_DIR="C:/Qt/6.8.0/mingw_64" MINGW_DIR="C:/Qt/Tools/mingw1310_64" ./build.sh
```

`build.sh` does a clean configure each time; pass `--keep` to reuse the cache.

## Manual build

```bash
cd qt-client
cmake -B build -S . -DCMAKE_PREFIX_PATH="<path-to-Qt>/<version>/<compiler>"
cmake --build build --config Release
```

Examples for `CMAKE_PREFIX_PATH`:
- Windows (MSVC): `C:/Qt/6.7.2/msvc2019_64`
- Windows (MinGW): `C:/Qt/6.7.2/mingw_64`
- Linux: usually auto-found; otherwise `~/Qt/6.7.2/gcc_64`

Or just open `CMakeLists.txt` in **Qt Creator** and press Run.

## Run

```bash
# Windows
build/Release/esp32cam_client.exe
# Linux / macOS
./build/esp32cam_client
```

1. The URL field is prefilled with the relay `/view` endpoint — edit it to your host.
2. Click **Connect**. You should see live video and an FPS readout.
3. It auto-reconnects if the connection drops.

## Notes

- The relay sends one JPEG per WebSocket binary message; `QImage::loadFromData`
  decodes it automatically.
- `/view` is currently open (no token). If you add viewer auth to the relay,
  append it to the URL, e.g. `wss://<host>/view?token=...`.
- Windows deployment: run `windeployqt` on the built `.exe` to bundle the Qt
  DLLs, or run from within Qt Creator.
