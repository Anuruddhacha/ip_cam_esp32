#pragma once

#include <cstddef>
#include <cstdint>

// Thread-safe "latest value" broadcast primitive: one producer (the camera
// capture task) publishes JPEG frames; any number of independent consumers
// (HTTP stream, snapshot endpoint, relay push) each poll for "is there a
// frame newer than the last one I saw". This is what decouples every
// consumer from the camera itself - nothing outside camera.cpp ever calls
// esp_camera_fb_get()/frame2jpg().
namespace frame_buffer {

// Must be called once before publish()/get_latest() are used.
void init();

// Producer side: takes ownership of buf (must be malloc'd) and frees
// whatever frame was previously published.
void publish(uint8_t *buf, size_t len);

// Consumer side: copies out the latest frame if it's newer than *last_seq,
// advancing *last_seq to match. Caller owns and must free() the returned
// buffer. Returns false (touching nothing) if no newer frame is available
// yet - pass *last_seq = 0 to always pick up whatever is newest.
bool get_latest(uint32_t *last_seq, uint8_t **out_buf, size_t *out_len);

}  // namespace frame_buffer
