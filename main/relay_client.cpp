#include "relay_client.h"

#include "config.h"
#include "frame_buffer.h"

extern "C" {
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_websocket_client.h"
#include "esp_crt_bundle.h"
}

#include <cstdlib>

namespace relay_client {
namespace {

esp_websocket_client_handle_t s_ws_client = nullptr;
volatile bool s_ws_connected = false;

void ws_event_handler(void *handler_args, esp_event_base_t base,
                      int32_t event_id, void *event_data)
{
    switch (event_id) {
    case WEBSOCKET_EVENT_CONNECTED:
        s_ws_connected = true;
        ESP_LOGI(config::TAG, "Relay connected");
        break;
    case WEBSOCKET_EVENT_DISCONNECTED:
        s_ws_connected = false;
        ESP_LOGW(config::TAG, "Relay disconnected");
        break;
    case WEBSOCKET_EVENT_ERROR:
        ESP_LOGW(config::TAG, "Relay connection error");
        break;
    default:
        break;
    }
}

/* Pushes whatever the capture task last produced to the relay. Does not
 * touch the camera itself. */
void push_task(void *arg)
{
    uint32_t last_seq = 0;
    while (true) {
        if (!s_ws_connected) {
            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }

        uint8_t *jpg_buf = nullptr;
        size_t jpg_len = 0;
        if (!frame_buffer::get_latest(&last_seq, &jpg_buf, &jpg_len)) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        int sent = esp_websocket_client_send_bin(
            s_ws_client, reinterpret_cast<const char *>(jpg_buf),
            jpg_len, pdMS_TO_TICKS(2000));
        if (sent < 0) {
            ESP_LOGW(config::TAG, "Relay frame send failed");
        }
        free(jpg_buf);
    }
}

}  // namespace

void init()
{
    if (!config::RELAY_ENABLED) {
        ESP_LOGW(config::TAG, "Relay disabled. Set RELAY_WS_URI and RELAY_ENABLED=true. "
                      "Local /stream still works on the LAN.");
        return;
    }

    esp_websocket_client_config_t cfg = {};
    cfg.uri                  = config::RELAY_WS_URI;
    cfg.buffer_size          = 8192;
    cfg.reconnect_timeout_ms = 5000;
    cfg.network_timeout_ms   = 10000;
    cfg.task_stack           = 6144;
    /* Validate the server's TLS cert against the bundled root CAs. Required
     * for wss:// to public hosts; harmless for plain ws://. Needs
     * CONFIG_MBEDTLS_CERTIFICATE_BUNDLE=y (set in sdkconfig.defaults). */
    cfg.crt_bundle_attach    = esp_crt_bundle_attach;

    s_ws_client = esp_websocket_client_init(&cfg);
    if (s_ws_client == nullptr) {
        ESP_LOGE(config::TAG, "Failed to init relay WebSocket client");
        return;
    }

    esp_websocket_register_events(s_ws_client, WEBSOCKET_EVENT_ANY,
                                  ws_event_handler, nullptr);
    esp_websocket_client_start(s_ws_client);

    xTaskCreate(push_task, "relay_push", 8192, nullptr, 5, nullptr);
    ESP_LOGI(config::TAG, "Relay client started -> %s", config::RELAY_WS_URI);
}

}  // namespace relay_client
