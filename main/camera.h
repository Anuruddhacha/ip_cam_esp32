#pragma once

extern "C" {
#include "esp_err.h"
}

// Owns the camera hardware. init() configures and starts the sensor;
// start_capture_task() launches the single task that is ever allowed to
// call esp_camera_fb_get()/frame2jpg() - it captures, JPEG-encodes, and
// publishes each frame via frame_buffer for anything else to read.
namespace camera {

esp_err_t init();
void start_capture_task();

}  // namespace camera
