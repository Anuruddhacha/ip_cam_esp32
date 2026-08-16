#include "frame_buffer.h"

extern "C" {
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
}

#include <cstdlib>
#include <cstring>

namespace frame_buffer {
namespace {

SemaphoreHandle_t s_mutex = nullptr;
uint8_t *s_buf = nullptr;
size_t s_len = 0;
uint32_t s_seq = 0;  // bumped on every new frame

}  // namespace

void init()
{
    s_mutex = xSemaphoreCreateMutex();
}

void publish(uint8_t *buf, size_t len)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    free(s_buf);
    s_buf = buf;
    s_len = len;
    s_seq++;
    xSemaphoreGive(s_mutex);
}

bool get_latest(uint32_t *last_seq, uint8_t **out_buf, size_t *out_len)
{
    bool got = false;
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    if (s_buf != nullptr && s_seq != *last_seq) {
        auto *copy = static_cast<uint8_t *>(malloc(s_len));
        if (copy != nullptr) {
            std::memcpy(copy, s_buf, s_len);
            *out_buf = copy;
            *out_len = s_len;
            *last_seq = s_seq;
            got = true;
        }
    }
    xSemaphoreGive(s_mutex);
    return got;
}

}  // namespace frame_buffer
