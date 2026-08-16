#include <cstdio>
#include <cstring>
#include <cstdlib>

extern "C" {
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_http_server.h"
#include "esp_psram.h"
#include "esp_camera.h"
#include "img_converters.h"
#include "esp_websocket_client.h"
#include "esp_crt_bundle.h"
}

namespace {

constexpr const char *WIFI_SSID = "SLT-4G_597084";
constexpr const char *WIFI_PASS = "A6FC8BA5";
constexpr const char *TAG = "ESP32_CAM";

/* Software JPEG quality for frame2jpg: 0-100, higher = better but larger and
 * slower to encode. Lower it to raise the frame rate (smaller frames encode
 * and upload faster). ~40-60 is a good speed/quality balance. */
constexpr int JPEG_QUALITY = 45;

/* Minimum delay between captures in capture_task (the only task that talks
 * to the camera). The real limit is JPEG encode time; this just caps the
 * max rate / yields CPU to other tasks. Lower = faster. Set to 0 to capture
 * as fast as the pipeline allows. Both /stream and the relay push task read
 * whatever capture_task last produced, so this one constant paces both. */
constexpr int CAPTURE_INTERVAL_MS = 20;

/* ---------------- Remote relay (WebSocket push) ----------------
 * The camera makes an OUTBOUND WebSocket connection to your public relay
 * server and pushes JPEG frames. Viewers connect to the relay (not to the
 * camera), so this works from any network without port-forwarding.
 *
 * 1. Deploy the relay in ./relay on a public host (VPS / free tier).
 * 2. Set RELAY_WS_URI (host + token) and flip RELAY_ENABLED to true.
 * 3. Use wss:// (TLS) for real deployments; see relay/README.md.
 */
constexpr bool RELAY_ENABLED = true;  // set true after configuring the URI below
constexpr const char *RELAY_WS_URI =
    "wss://ipcamserver-17qt9dgm.b4a.run/ingest?token=pick-a-secret";

/* ---------------- AI-Thinker ESP32-CAM pin map ---------------- */
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

/* MJPEG multipart streaming boundaries */
constexpr const char *STREAM_CONTENT_TYPE =
    "multipart/x-mixed-replace;boundary=frame";
constexpr const char *STREAM_BOUNDARY = "\r\n--frame\r\n";
constexpr const char *STREAM_PART =
    "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

/* ---------------- Camera ---------------- */

esp_err_t camera_init()
{
    camera_config_t config = {};
    config.pin_pwdn     = PWDN_GPIO_NUM;
    config.pin_reset    = RESET_GPIO_NUM;
    config.pin_xclk     = XCLK_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_d7       = Y9_GPIO_NUM;
    config.pin_d6       = Y8_GPIO_NUM;
    config.pin_d5       = Y7_GPIO_NUM;
    config.pin_d4       = Y6_GPIO_NUM;
    config.pin_d3       = Y5_GPIO_NUM;
    config.pin_d2       = Y4_GPIO_NUM;
    config.pin_d1       = Y3_GPIO_NUM;
    config.pin_d0       = Y2_GPIO_NUM;
    config.pin_vsync    = VSYNC_GPIO_NUM;
    config.pin_href     = HREF_GPIO_NUM;
    config.pin_pclk     = PCLK_GPIO_NUM;

    /* Raw RGB565 is bandwidth heavy (2 bytes/pixel). A 20 MHz pixel clock
     * overruns the camera DMA (EV-EOF-OVF), so run XCLK slower. */
    config.xclk_freq_hz = 10000000;  // 10 MHz keeps RGB565 DMA stable
    config.ledc_timer   = LEDC_TIMER_0;
    config.ledc_channel = LEDC_CHANNEL_0;

    /* This board uses a GC2145 sensor, which has NO hardware JPEG encoder.
     * We capture raw RGB565 and encode JPEG in software (frame2jpg).
     * Software JPEG is CPU heavy and RGB565 DMA bandwidth is limited, so
     * keep the frame size modest (QVGA) for reliable, overflow-free frames. */
    config.pixel_format = PIXFORMAT_RGB565;
    config.grab_mode    = CAMERA_GRAB_LATEST;
    config.fb_location  = CAMERA_FB_IN_PSRAM;

    if (esp_psram_is_initialized()) {
        config.frame_size = FRAMESIZE_QVGA;   // 320x240
        config.fb_count   = 2;
    } else {
        config.frame_size  = FRAMESIZE_QVGA;  // 320x240
        config.fb_count    = 1;
        config.fb_location = CAMERA_FB_IN_DRAM;
    }

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera init failed: 0x%x", err);
        return err;
    }

    /* Sensor tweaks: many modules are mounted upside-down. */
    sensor_t *s = esp_camera_sensor_get();
    if (s != nullptr) {
        s->set_vflip(s, 1);
        s->set_hmirror(s, 1);
        s->set_brightness(s, 1);
        s->set_saturation(s, 0);
    }

    ESP_LOGI(TAG, "Camera initialized");
    return ESP_OK;
}

/* ---------------- WiFi ---------------- */

void wifi_event_handler(void *arg, esp_event_base_t event_base,
                        int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "WiFi disconnected, reconnecting...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        auto *event = static_cast<ip_event_got_ip_t *>(event_data);
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "Open http://" IPSTR "/ in your browser", IP2STR(&event->ip_info.ip));
    }
}

void wifi_init()
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                               &wifi_event_handler, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                               &wifi_event_handler, nullptr));

    wifi_config_t wifi_config = {};
    std::strncpy(reinterpret_cast<char *>(wifi_config.sta.ssid), WIFI_SSID,
                 sizeof(wifi_config.sta.ssid));
    std::strncpy(reinterpret_cast<char *>(wifi_config.sta.password), WIFI_PASS,
                 sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi started, connecting to \"%s\"", WIFI_SSID);
}

/* ---------------- Shared frame buffer ----------------
 * capture_task is the ONLY task that ever calls esp_camera_fb_get() /
 * frame2jpg(). The local MJPEG stream, the /capture endpoint, and the
 * relay push task all just read the latest encoded JPEG from here instead
 * of independently capturing+encoding. That used to double the camera/CPU
 * work whenever a LAN viewer and the relay were both active, and the
 * esp32-camera driver isn't documented as safe for concurrent fb_get()
 * calls from multiple tasks.
 */
struct SharedFrame {
    SemaphoreHandle_t mutex = nullptr;
    uint8_t *buf = nullptr;
    size_t len = 0;
    uint32_t seq = 0;  // bumped on every new frame
};

SharedFrame s_frame;

/* Copies out the latest frame if it's newer than *last_seq, updating
 * *last_seq. Caller owns and must free() the returned buffer. Returns
 * false (touching nothing) if no newer frame is available yet. */
bool frame_buffer_get_latest(uint32_t *last_seq, uint8_t **out_buf, size_t *out_len)
{
    bool got = false;
    xSemaphoreTake(s_frame.mutex, portMAX_DELAY);
    if (s_frame.buf != nullptr && s_frame.seq != *last_seq) {
        auto *copy = static_cast<uint8_t *>(malloc(s_frame.len));
        if (copy != nullptr) {
            std::memcpy(copy, s_frame.buf, s_frame.len);
            *out_buf = copy;
            *out_len = s_frame.len;
            *last_seq = s_frame.seq;
            got = true;
        }
    }
    xSemaphoreGive(s_frame.mutex);
    return got;
}

/* The only task that touches the camera: capture, JPEG-encode, publish. */
void capture_task(void *arg)
{
    while (true) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (fb == nullptr) {
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        uint8_t *jpg_buf = nullptr;
        size_t jpg_len = 0;
        bool ok = frame2jpg(fb, JPEG_QUALITY, &jpg_buf, &jpg_len);
        esp_camera_fb_return(fb);

        if (ok) {
            xSemaphoreTake(s_frame.mutex, portMAX_DELAY);
            free(s_frame.buf);
            s_frame.buf = jpg_buf;
            s_frame.len = jpg_len;
            s_frame.seq++;
            xSemaphoreGive(s_frame.mutex);
        } else {
            ESP_LOGW(TAG, "JPEG encode failed");
        }

        vTaskDelay(pdMS_TO_TICKS(CAPTURE_INTERVAL_MS));
    }
}

/* ---------------- HTTP handlers ---------------- */

esp_err_t index_handler(httpd_req_t *req)
{
    static const char html[] =
        "<!DOCTYPE html><html><head><meta charset=\"utf-8\">"
        "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
        "<title>ESP32-CAM</title>"
        "<style>body{font-family:sans-serif;text-align:center;background:#111;color:#eee;margin:0}"
        "h1{padding:12px}img{max-width:100%;height:auto;border:1px solid #333}"
        "a{color:#4ea1ff}</style></head>"
        "<body><h1>ESP32-CAM Live Stream</h1>"
        "<img src=\"/stream\"><p><a href=\"/capture\">Capture a still</a></p>"
        "</body></html>";

    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, html, HTTPD_RESP_USE_STRLEN);
}

esp_err_t capture_handler(httpd_req_t *req)
{
    uint32_t seq = 0;  // always wants whatever is newest
    uint8_t *jpg_buf = nullptr;
    size_t jpg_len = 0;
    if (!frame_buffer_get_latest(&seq, &jpg_buf, &jpg_len)) {
        ESP_LOGE(TAG, "No frame available yet");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "image/jpeg");
    httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");

    esp_err_t res = httpd_resp_send(req, reinterpret_cast<const char *>(jpg_buf), jpg_len);
    free(jpg_buf);
    return res;
}

esp_err_t stream_handler(httpd_req_t *req)
{
    esp_err_t res = httpd_resp_set_type(req, STREAM_CONTENT_TYPE);
    if (res != ESP_OK) {
        return res;
    }

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    char part_buf[64];
    uint32_t last_seq = 0;

    while (true) {
        uint8_t *jpg_buf = nullptr;
        size_t jpg_len = 0;
        if (!frame_buffer_get_latest(&last_seq, &jpg_buf, &jpg_len)) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        res = httpd_resp_send_chunk(req, STREAM_BOUNDARY, std::strlen(STREAM_BOUNDARY));
        if (res == ESP_OK) {
            int len = std::snprintf(part_buf, sizeof(part_buf), STREAM_PART, jpg_len);
            res = httpd_resp_send_chunk(req, part_buf, len);
        }
        if (res == ESP_OK) {
            res = httpd_resp_send_chunk(req,
                reinterpret_cast<const char *>(jpg_buf), jpg_len);
        }

        free(jpg_buf);

        if (res != ESP_OK) {
            ESP_LOGW(TAG, "Stream client disconnected");
            break;
        }
    }

    return res;
}

httpd_handle_t start_webserver()
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.lru_purge_enable = true;
    config.stack_size = 8192;

    httpd_handle_t server = nullptr;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t index_uri = {
            .uri = "/", .method = HTTP_GET,
            .handler = index_handler, .user_ctx = nullptr,
        };
        httpd_uri_t stream_uri = {
            .uri = "/stream", .method = HTTP_GET,
            .handler = stream_handler, .user_ctx = nullptr,
        };
        httpd_uri_t capture_uri = {
            .uri = "/capture", .method = HTTP_GET,
            .handler = capture_handler, .user_ctx = nullptr,
        };

        httpd_register_uri_handler(server, &index_uri);
        httpd_register_uri_handler(server, &stream_uri);
        httpd_register_uri_handler(server, &capture_uri);

        ESP_LOGI(TAG, "HTTP server started");
    } else {
        ESP_LOGE(TAG, "Failed to start HTTP server");
    }

    return server;
}

/* ---------------- Remote relay client ---------------- */

esp_websocket_client_handle_t s_ws_client = nullptr;
volatile bool s_ws_connected = false;

void ws_event_handler(void *handler_args, esp_event_base_t base,
                      int32_t event_id, void *event_data)
{
    switch (event_id) {
    case WEBSOCKET_EVENT_CONNECTED:
        s_ws_connected = true;
        ESP_LOGI(TAG, "Relay connected");
        break;
    case WEBSOCKET_EVENT_DISCONNECTED:
        s_ws_connected = false;
        ESP_LOGW(TAG, "Relay disconnected");
        break;
    case WEBSOCKET_EVENT_ERROR:
        ESP_LOGW(TAG, "Relay connection error");
        break;
    default:
        break;
    }
}

/* Pushes whatever capture_task last produced to the relay. Does not touch
 * the camera itself. */
void relay_push_task(void *arg)
{
    uint32_t last_seq = 0;
    while (true) {
        if (!s_ws_connected) {
            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }

        uint8_t *jpg_buf = nullptr;
        size_t jpg_len = 0;
        if (!frame_buffer_get_latest(&last_seq, &jpg_buf, &jpg_len)) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        int sent = esp_websocket_client_send_bin(
            s_ws_client, reinterpret_cast<const char *>(jpg_buf),
            jpg_len, pdMS_TO_TICKS(2000));
        if (sent < 0) {
            ESP_LOGW(TAG, "Relay frame send failed");
        }
        free(jpg_buf);
    }
}

void relay_init()
{
    if (!RELAY_ENABLED) {
        ESP_LOGW(TAG, "Relay disabled. Set RELAY_WS_URI and RELAY_ENABLED=true. "
                      "Local /stream still works on the LAN.");
        return;
    }

    esp_websocket_client_config_t cfg = {};
    cfg.uri                 = RELAY_WS_URI;
    cfg.buffer_size         = 8192;
    cfg.reconnect_timeout_ms = 5000;
    cfg.network_timeout_ms  = 10000;
    cfg.task_stack          = 6144;
    /* Validate the server's TLS cert against the bundled root CAs. Required
     * for wss:// to public hosts; harmless for plain ws://. Needs
     * CONFIG_MBEDTLS_CERTIFICATE_BUNDLE=y (set in sdkconfig.defaults). */
    cfg.crt_bundle_attach   = esp_crt_bundle_attach;

    s_ws_client = esp_websocket_client_init(&cfg);
    if (s_ws_client == nullptr) {
        ESP_LOGE(TAG, "Failed to init relay WebSocket client");
        return;
    }

    esp_websocket_register_events(s_ws_client, WEBSOCKET_EVENT_ANY,
                                  ws_event_handler, nullptr);
    esp_websocket_client_start(s_ws_client);

    xTaskCreate(relay_push_task, "relay_push", 8192, nullptr, 5, nullptr);
    ESP_LOGI(TAG, "Relay client started -> %s", RELAY_WS_URI);
}

}  // namespace

extern "C" void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    if (camera_init() != ESP_OK) {
        ESP_LOGE(TAG, "Camera failed to initialize, halting");
        return;
    }

    s_frame.mutex = xSemaphoreCreateMutex();
    /* Priority above relay_push_task/httpd (5) so the producer isn't
     * starved by its consumers. */
    xTaskCreate(capture_task, "cam_capture", 8192, nullptr, 6, nullptr);

    wifi_init();
    start_webserver();
    relay_init();

    ESP_LOGI(TAG, "ESP32-CAM ready");
}
