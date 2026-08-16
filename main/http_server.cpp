#include "http_server.h"

#include "config.h"
#include "frame_buffer.h"

extern "C" {
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
}

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace http_server {
namespace {

/* MJPEG multipart streaming boundaries */
constexpr const char *STREAM_CONTENT_TYPE =
    "multipart/x-mixed-replace;boundary=frame";
constexpr const char *STREAM_BOUNDARY = "\r\n--frame\r\n";
constexpr const char *STREAM_PART =
    "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

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
    if (!frame_buffer::get_latest(&seq, &jpg_buf, &jpg_len)) {
        ESP_LOGE(config::TAG, "No frame available yet");
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
        if (!frame_buffer::get_latest(&last_seq, &jpg_buf, &jpg_len)) {
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
            ESP_LOGW(config::TAG, "Stream client disconnected");
            break;
        }
    }

    return res;
}

}  // namespace

httpd_handle_t start()
{
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.server_port = 80;
    cfg.lru_purge_enable = true;
    cfg.stack_size = 8192;

    httpd_handle_t server = nullptr;

    if (httpd_start(&server, &cfg) == ESP_OK) {
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

        ESP_LOGI(config::TAG, "HTTP server started");
    } else {
        ESP_LOGE(config::TAG, "Failed to start HTTP server");
    }

    return server;
}

}  // namespace http_server
