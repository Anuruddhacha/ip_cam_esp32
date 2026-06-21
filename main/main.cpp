#include <cstdio>
#include <cstring>
#include <cstdlib>

extern "C" {
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_http_server.h"
}

namespace {

constexpr const char *WIFI_SSID = "SLT-4G_597084";
constexpr const char *WIFI_PASS = "A6FC8BA5";
constexpr const char *TAG = "DUMMY_CAM";

void wifi_event_handler(void *arg, esp_event_base_t event_base,
                        int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    }
}

void wifi_init()
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                               &wifi_event_handler, nullptr));

    wifi_config_t wifi_config = {};
    std::strncpy(reinterpret_cast<char *>(wifi_config.sta.ssid), WIFI_SSID,
                 sizeof(wifi_config.sta.ssid));
    std::strncpy(reinterpret_cast<char *>(wifi_config.sta.password), WIFI_PASS,
                 sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi initialized");
}

esp_err_t stream_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "multipart/x-mixed-replace; boundary=frame");

    int frame = 0;

    while (true) {
        char buffer[512];

        int len = std::snprintf(buffer, sizeof(buffer),
            "--frame\r\n"
            "Content-Type: text/plain\r\n\r\n"
            "ESP32 DUMMY CAMERA FRAME: %d\r\n\r\n",
            frame++);

        esp_err_t res = httpd_resp_send_chunk(req, buffer, len);

        if (res != ESP_OK) {
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }

    return ESP_OK;
}

httpd_handle_t start_webserver()
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = nullptr;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t stream_uri = {
            .uri = "/stream",
            .method = HTTP_GET,
            .handler = stream_handler,
            .user_ctx = nullptr,
        };

        httpd_register_uri_handler(server, &stream_uri);
        ESP_LOGI(TAG, "HTTP server started");
    }

    return server;
}

}  // namespace

extern "C" void app_main(void)
{
    wifi_init();
    start_webserver();

    ESP_LOGI(TAG, "Dummy IP Camera Ready");
}
