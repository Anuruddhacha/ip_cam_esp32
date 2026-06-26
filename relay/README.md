# ESP32-CAM WebSocket Relay

Lets you watch the ESP32-CAM from **any network**, without port-forwarding.

```
ESP32-CAM ──(outbound WS, pushes JPEG)──► Relay (this server, public host) ──(WS)──► Browser viewers
```

The camera dials out to this relay, so it works behind NAT/CGNAT. Viewers open
the relay's web page; they never connect to the camera directly.

## 1. Run the relay on a public host

You need a host with a public IP/hostname (a cheap VPS, or a free tier like
Fly.io / Railway / Render). Requires Node.js 18+.

```bash
cd relay
npm install
TOKEN=pick-a-secret PORT=8080 npm start
```

- Viewer page: `http://<host>:8080/`
- Camera ingest URI: `ws://<host>:8080/ingest?token=pick-a-secret`

`TOKEN` protects the `/ingest` endpoint so only your camera can publish.

## 2. Point the ESP32-CAM at the relay

In `main/main.cpp`, set:

```cpp
constexpr const char *RELAY_WS_URI =
    "ws://<host>:8080/ingest?token=pick-a-secret";
```

Rebuild and flash:

```bash
idf.py build
idf.py -p COM5 flash monitor
```

You should see `Relay connected` in the serial log, and live video on the
viewer page from anywhere.

## 3. Use TLS (recommended for real use)

Plain `ws://` sends frames unencrypted. For production:

- Put the relay behind a reverse proxy (Caddy/Nginx) or a platform that
  terminates HTTPS, so the public URL is `https://` / `wss://`.
- Change the ESP32 URI to `wss://<host>/ingest?token=...`.
- In `relay_init()` (firmware), enable the certificate bundle:
  - add `espressif/esp-tls` cert bundle, set `cfg.cert_pem` or
    `esp_websocket_client` with `transport = WEBSOCKET_TRANSPORT_OVER_SSL`.

Ask and I can wire up the `wss://` path in the firmware.

## Notes / limits

- The ESP32-CAM (GC2145, software JPEG, QVGA) realistically pushes a few fps.
  Tune `RELAY_FRAME_INTERVAL_MS` and `JPEG_QUALITY` in `main.cpp`.
- One camera, many viewers is fine — the relay fans out the latest frame.
- `/healthz` returns `ok` for uptime checks.
