#pragma once

extern "C" {
#include "esp_http_server.h"
}

// Local, unauthenticated LAN server: "/" (viewer page), "/stream" (MJPEG
// multipart), "/capture" (single JPEG). All handlers read from
// frame_buffer - none of them touch the camera directly.
namespace http_server {

httpd_handle_t start();

}  // namespace http_server
