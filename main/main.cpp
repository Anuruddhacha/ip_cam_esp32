#include <cstdio>
#include <cstring>
#include <cstdlib>

extern "C" {
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_http_server.h"
#include "esp_psram.h"
#include "esp_camera.h"
#include "img_converters.h"
}

namespace {

constexpr const char *WIFI_SSID = "SLT-4G_597084";
constexpr const char *WIFI_PASS = "A6FC8BA5";
constexpr const char *TAG = "ESP32_CAM";

/* Software JPEG quality for frame2jpg: 0-100, higher = better/larger. */
constexpr int JPEG_QUALITY = 80;

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
    camera_fb_t *fb = esp_camera_fb_get();
    if (fb == nullptr) {
        ESP_LOGE(TAG, "Camera capture failed");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    /* GC2145 gives us raw RGB565; encode to JPEG in software. */
    uint8_t *jpg_buf = nullptr;
    size_t jpg_len = 0;
    bool ok = frame2jpg(fb, JPEG_QUALITY, &jpg_buf, &jpg_len);
    esp_camera_fb_return(fb);

    if (!ok) {
        ESP_LOGE(TAG, "JPEG encode failed");
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

    while (true) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (fb == nullptr) {
            ESP_LOGE(TAG, "Camera capture failed");
            res = ESP_FAIL;
            break;
        }

        uint8_t *jpg_buf = nullptr;
        size_t jpg_len = 0;
        bool ok = frame2jpg(fb, JPEG_QUALITY, &jpg_buf, &jpg_len);
        esp_camera_fb_return(fb);

        if (!ok) {
            ESP_LOGE(TAG, "JPEG encode failed");
            res = ESP_FAIL;
            break;
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

    wifi_init();
    start_webserver();

    ESP_LOGI(TAG, "ESP32-CAM ready");
}
