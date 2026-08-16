#include "camera.h"

#include "config.h"
#include "frame_buffer.h"

extern "C" {
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_psram.h"
#include "esp_camera.h"
#include "img_converters.h"
#include "esp_timer.h"
}

namespace camera {
namespace {

/* The only task that touches the camera: capture, JPEG-encode, publish. */
void capture_task(void *arg)
{
    uint32_t frames_since_log = 0;
    uint64_t bytes_since_log = 0;
    int64_t window_start_us = esp_timer_get_time();

    while (true) {
        int64_t t0 = esp_timer_get_time();

        camera_fb_t *fb = esp_camera_fb_get();
        if (fb == nullptr) {
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        uint8_t *jpg_buf = nullptr;
        size_t jpg_len = 0;
        bool ok = frame2jpg(fb, config::JPEG_QUALITY, &jpg_buf, &jpg_len);
        esp_camera_fb_return(fb);

        if (ok) {
            frame_buffer::publish(jpg_buf, jpg_len);
            frames_since_log++;
            bytes_since_log += jpg_len;
        } else {
            ESP_LOGW(config::TAG, "JPEG encode failed");
        }

        /* Real achieved fps/frame-size, logged every ~3s. This is the
         * number that tells us whether slowness is happening here (capture
         * + software JPEG encode) or further downstream (WiFi upload,
         * relay, viewer). Compare it against what the Qt client / browser
         * actually display. */
        int64_t now = esp_timer_get_time();
        if (now - window_start_us >= 3000000) {
            double secs = (now - window_start_us) / 1e6;
            ESP_LOGI(config::TAG, "capture: %.1f fps, avg %.1f KB/frame, last capture+encode %.1f ms",
                     frames_since_log / secs,
                     frames_since_log ? (bytes_since_log / 1024.0) / frames_since_log : 0.0,
                     (now - t0) / 1000.0);
            frames_since_log = 0;
            bytes_since_log = 0;
            window_start_us = now;
        }

        vTaskDelay(pdMS_TO_TICKS(config::CAPTURE_INTERVAL_MS));
    }
}

}  // namespace

esp_err_t init()
{
    camera_config_t cfg = {};
    cfg.pin_pwdn     = config::PWDN_GPIO_NUM;
    cfg.pin_reset    = config::RESET_GPIO_NUM;
    cfg.pin_xclk     = config::XCLK_GPIO_NUM;
    cfg.pin_sccb_sda = config::SIOD_GPIO_NUM;
    cfg.pin_sccb_scl = config::SIOC_GPIO_NUM;
    cfg.pin_d7       = config::Y9_GPIO_NUM;
    cfg.pin_d6       = config::Y8_GPIO_NUM;
    cfg.pin_d5       = config::Y7_GPIO_NUM;
    cfg.pin_d4       = config::Y6_GPIO_NUM;
    cfg.pin_d3       = config::Y5_GPIO_NUM;
    cfg.pin_d2       = config::Y4_GPIO_NUM;
    cfg.pin_d1       = config::Y3_GPIO_NUM;
    cfg.pin_d0       = config::Y2_GPIO_NUM;
    cfg.pin_vsync    = config::VSYNC_GPIO_NUM;
    cfg.pin_href     = config::HREF_GPIO_NUM;
    cfg.pin_pclk     = config::PCLK_GPIO_NUM;

    /* Raw RGB565 is bandwidth heavy (2 bytes/pixel). A 20 MHz pixel clock
     * overruns the camera DMA (EV-EOF-OVF), so run XCLK slower. */
    cfg.xclk_freq_hz = 10000000;  // 10 MHz keeps RGB565 DMA stable
    cfg.ledc_timer   = LEDC_TIMER_0;
    cfg.ledc_channel = LEDC_CHANNEL_0;

    /* This board uses a GC2145 sensor, which has NO hardware JPEG encoder.
     * We capture raw RGB565 and encode JPEG in software (frame2jpg). That's
     * CPU heavy, and most of the cost is reading/color-converting the raw
     * frame out of PSRAM before it's even compressed - at QVGA (320x240,
     * 150 KB/frame raw) that was measured at ~150-250ms/frame (~4-5 fps)
     * and even overran the camera DMA (EV-VSYNC-OVF) because the encoder
     * couldn't free buffers fast enough. QQVGA has 1/4 the raw pixels, so
     * it cuts that read+convert cost roughly 4x. */
    cfg.pixel_format = PIXFORMAT_RGB565;
    cfg.grab_mode    = CAMERA_GRAB_LATEST;
    cfg.fb_location  = CAMERA_FB_IN_PSRAM;

    if (esp_psram_is_initialized()) {
        cfg.frame_size = FRAMESIZE_QQVGA;   // 160x120
        cfg.fb_count   = 2;
    } else {
        cfg.frame_size  = FRAMESIZE_QQVGA;  // 160x120
        cfg.fb_count    = 1;
        cfg.fb_location = CAMERA_FB_IN_DRAM;
    }

    esp_err_t err = esp_camera_init(&cfg);
    if (err != ESP_OK) {
        ESP_LOGE(config::TAG, "Camera init failed: 0x%x", err);
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

    ESP_LOGI(config::TAG, "Camera initialized");
    return ESP_OK;
}

void start_capture_task()
{
    /* Priority above relay_push_task/httpd (5) so the producer isn't
     * starved by its consumers. Pinned to core 1 (APP_CPU) so the
     * CPU-heavy software JPEG encode doesn't fight the WiFi/lwIP stack,
     * which IDF keeps on core 0 (PRO_CPU), for cycles. */
    xTaskCreatePinnedToCore(capture_task, "cam_capture", 8192, nullptr, 6, nullptr, 1);
}

}  // namespace camera
