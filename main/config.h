#pragma once

// Central place for firmware configuration: WiFi credentials, the relay
// endpoint, camera/capture tuning knobs, and the pin map. Edit this file to
// point the firmware at your network/relay or adjust image quality vs.
// frame rate - no need to dig through module implementations to find a
// constant.
namespace config {

constexpr const char *TAG = "ESP32_CAM";

// ---------------- WiFi ----------------
constexpr const char *WIFI_SSID = "SLT-4G_597084";
constexpr const char *WIFI_PASS = "A6FC8BA5";

// ---------------- Camera / capture tuning ----------------

/* Software JPEG quality for frame2jpg: 0-100, higher = better but larger and
 * slower to encode. Lower it to raise the frame rate (smaller frames encode
 * and upload faster). ~40-60 is a good speed/quality balance. */
constexpr int JPEG_QUALITY = 45;

/* Minimum delay between captures in the capture task (the only task that
 * talks to the camera). The real limit is JPEG encode time; this just caps
 * the max rate / yields CPU to other tasks. Lower = faster. Set to 0 to
 * capture as fast as the pipeline allows. Both /stream and the relay push
 * task read whatever the capture task last produced, so this one constant
 * paces both. */
constexpr int CAPTURE_INTERVAL_MS = 20;

// ---------------- Remote relay (WebSocket push) ----------------
/* The camera makes an OUTBOUND WebSocket connection to your public relay
 * server and pushes JPEG frames. Viewers connect to the relay (not to the
 * camera), so this works from any network without port-forwarding.
 *
 * 1. Deploy the relay in ./relay on a public host (VPS / free tier).
 * 2. Set RELAY_WS_URI (host + token) and flip RELAY_ENABLED to true.
 * 3. Use wss:// (TLS) for real deployments; see relay/README.md.
 */
constexpr bool RELAY_ENABLED = true;
constexpr const char *RELAY_WS_URI =
    "wss://ipcamserver-5cmchklo.b4a.run/ingest?token=pick-a-secret";

// ---------------- AI-Thinker ESP32-CAM pin map ----------------
constexpr int PWDN_GPIO_NUM  = 32;
constexpr int RESET_GPIO_NUM = -1;
constexpr int XCLK_GPIO_NUM  = 0;
constexpr int SIOD_GPIO_NUM  = 26;
constexpr int SIOC_GPIO_NUM  = 27;
constexpr int Y9_GPIO_NUM    = 35;
constexpr int Y8_GPIO_NUM    = 34;
constexpr int Y7_GPIO_NUM    = 39;
constexpr int Y6_GPIO_NUM    = 36;
constexpr int Y5_GPIO_NUM    = 21;
constexpr int Y4_GPIO_NUM    = 19;
constexpr int Y3_GPIO_NUM    = 18;
constexpr int Y2_GPIO_NUM    = 5;
constexpr int VSYNC_GPIO_NUM = 25;
constexpr int HREF_GPIO_NUM  = 23;
constexpr int PCLK_GPIO_NUM  = 22;

}  // namespace config
