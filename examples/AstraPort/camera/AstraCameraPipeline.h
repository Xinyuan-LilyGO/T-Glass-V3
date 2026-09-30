#pragma once

#include <cstddef>
#include <cstdint>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "AstraCameraFrame.h"

class AstraCameraPipeline {
public:
    explicit AstraCameraPipeline(bool cameraReady);
    ~AstraCameraPipeline();

    AstraCameraPipeline(const AstraCameraPipeline &) = delete;
    AstraCameraPipeline &operator=(const AstraCameraPipeline &) = delete;

    bool available() const;
    bool copyJpegFrame(std::uint8_t *destination,
                       std::size_t capacity,
                       astra_camera::FrameInfo &info,
                       TickType_t timeout = pdMS_TO_TICKS(100));

private:
    bool cameraReady_ = false;
    SemaphoreHandle_t mutex_ = nullptr;
};
