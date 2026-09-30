#include "AstraCameraPipeline.h"

#include <esp_camera.h>

AstraCameraPipeline::AstraCameraPipeline(bool cameraReady)
    : cameraReady_(cameraReady),
      mutex_(xSemaphoreCreateMutex()) {
    if (mutex_ == nullptr) {
        cameraReady_ = false;
    }
}

AstraCameraPipeline::~AstraCameraPipeline() {
    if (mutex_ != nullptr) {
        vSemaphoreDelete(mutex_);
        mutex_ = nullptr;
    }
}

bool AstraCameraPipeline::available() const {
    return cameraReady_ && mutex_ != nullptr;
}

bool AstraCameraPipeline::copyJpegFrame(std::uint8_t *destination,
                                         std::size_t capacity,
                                         astra_camera::FrameInfo &info,
                                         TickType_t timeout) {
    if (!available() || destination == nullptr || capacity == 0) {
        return false;
    }
    if (xSemaphoreTake(mutex_, timeout) != pdTRUE) {
        return false;
    }

    camera_fb_t *frame = esp_camera_fb_get();
    bool copied = false;
    if (frame != nullptr) {
        copied = astra_camera::copyJpegBytes(destination,
                                              capacity,
                                              frame->buf,
                                              frame->len,
                                              frame->width,
                                              frame->height,
                                              info,
                                              frame->format == PIXFORMAT_JPEG);
        esp_camera_fb_return(frame);
    }

    xSemaphoreGive(mutex_);
    return copied;
}
