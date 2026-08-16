#include "config.h"
#include "frame_buffer.h"
#include "camera.h"
#include "wifi_manager.h"
#include "http_server.h"
#include "relay_client.h"

extern "C" {
#include "esp_log.h"
#include "nvs_flash.h"
}

// Composition root: brings modules up in dependency order and wires
// nothing else. Every module only ever knows about frame_buffer (the
// shared "latest frame" primitive) and config (shared tunables/secrets) -
// never about each other. See each header for what it owns:
//   camera        - the only code that talks to the camera hardware
//   frame_buffer  - thread-safe "latest JPEG frame" hand-off
//   wifi_manager  - station WiFi connect/reconnect
//   http_server   - local LAN viewer/stream/capture endpoints
//   relay_client  - outbound push to the public relay
extern "C" void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    if (camera::init() != ESP_OK) {
        ESP_LOGE(config::TAG, "Camera failed to initialize, halting");
        return;
    }

    frame_buffer::init();
    camera::start_capture_task();

    wifi_manager::init();
    http_server::start();
    relay_client::init();

    ESP_LOGI(config::TAG, "ESP32-CAM ready");
}
