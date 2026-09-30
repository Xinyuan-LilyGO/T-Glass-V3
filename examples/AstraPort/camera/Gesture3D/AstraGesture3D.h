#pragma once

#include <cstddef>
#include <cstdint>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include <LilyGo_GlassV3.h>

#include "../AstraCameraPipeline.h"
#include "AstraGesture3DRenderer.h"
#include "AstraGesturePolicy.h"

using AstraGestureDigit = astra_gesture::Digit;

class AstraGesture3D {
public:
    AstraGesture3D(AstraCameraPipeline &camera, LilyGo_Glass &glass);
    ~AstraGesture3D();

    AstraGesture3D(const AstraGesture3D &) = delete;
    AstraGesture3D &operator=(const AstraGesture3D &) = delete;

    bool start();
    void stop();
    void update();
    void render();

    bool isActive() const;
    bool isAvailable() const;
    AstraGestureDigit currentDigit() const;
    bool takeNewDigit(AstraGestureDigit &digit);

private:
    struct GestureEvent {
        char category[24] = {};
        float score = 0.0F;
        std::uint32_t timestamp = 0;
    };

    static void taskEntry(void *context);
    void run();
    void releaseBuffers();

    AstraCameraPipeline &camera_;
    AstraGesture3DRenderer renderer_;
    QueueHandle_t eventQueue_ = nullptr;
    SemaphoreHandle_t stopped_ = nullptr;
    TaskHandle_t task_ = nullptr;
    std::uint8_t *jpegBuffer_ = nullptr;
    std::uint8_t *rgbBuffer_ = nullptr;
    volatile bool stopRequested_ = false;
    bool active_ = false;
    bool available_ = false;
    astra_gesture::Digit currentDigit_ = astra_gesture::Digit::None;
    bool digitPending_ = false;
    astra_gesture::Debouncer debouncer_{200};
};
