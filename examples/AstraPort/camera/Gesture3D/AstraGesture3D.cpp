#include "AstraGesture3D.h"

#include <algorithm>
#include <cstring>
#include <list>
#include <new>
#include <vector>

#include <Arduino.h>
#include <esp_heap_caps.h>
#include <esp_log.h>
#include <img_converters.h>

#include "hand_detect.hpp"
#include "hand_gesture_recognition.hpp"

namespace {

constexpr char kLogTag[] = "astra_gesture_3d";
constexpr std::uint16_t kFrameWidth = 320;
constexpr std::uint16_t kFrameHeight = 240;
constexpr std::size_t kJpegBufferSize = 96U * 1024U;
constexpr std::size_t kRgbBufferSize =
    static_cast<std::size_t>(kFrameWidth) * kFrameHeight * 3U;
constexpr std::size_t kModelWorkspaceMinimum = 280U * 1024U;
constexpr std::size_t kClassifyBurstFrames = 2;
constexpr std::uint32_t kTaskStackDepth = 12U * 1024U;
[[maybe_unused]] constexpr const char *kGestureLabels[] = {
    "one", "two", "three", "four", "five"};

void *allocatePsram(std::size_t bytes) {
    void *buffer = heap_caps_aligned_alloc(16, bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (buffer == nullptr) {
        buffer = ps_malloc(bytes);
    }
    return buffer;
}

} // namespace

AstraGesture3D::AstraGesture3D(AstraCameraPipeline &camera, LilyGo_Glass &glass)
    : camera_(camera), renderer_(glass) {
}

AstraGesture3D::~AstraGesture3D() {
    stop();
}

bool AstraGesture3D::start() {
    if (active_) {
        return true;
    }
    if (!camera_.available()) {
        return false;
    }

    eventQueue_ = xQueueCreate(1, sizeof(GestureEvent));
    stopped_ = xSemaphoreCreateBinary();
    jpegBuffer_ = static_cast<std::uint8_t *>(allocatePsram(kJpegBufferSize));
    rgbBuffer_ = static_cast<std::uint8_t *>(allocatePsram(kRgbBufferSize));
    if (eventQueue_ == nullptr || stopped_ == nullptr || jpegBuffer_ == nullptr ||
        rgbBuffer_ == nullptr || !renderer_.begin()) {
        stop();
        return false;
    }

    currentDigit_ = astra_gesture::Digit::None;
    digitPending_ = false;
    debouncer_.reset();
    stopRequested_ = false;
    available_ = true;
    active_ = true;

    const BaseType_t created = xTaskCreatePinnedToCore(taskEntry,
                                                       "astra_gesture_3d",
                                                       kTaskStackDepth,
                                                       this,
                                                       1,
                                                       &task_,
                                                       0);
    if (created != pdPASS) {
        active_ = false;
        available_ = false;
        stop();
        return false;
    }
    return true;
}

void AstraGesture3D::stop() {
    if (task_ != nullptr) {
        stopRequested_ = true;
        xTaskNotifyGive(task_);
        if (stopped_ != nullptr) {
            xSemaphoreTake(stopped_, portMAX_DELAY);
        }
        task_ = nullptr;
    }

    active_ = false;
    available_ = false;
    currentDigit_ = astra_gesture::Digit::None;
    digitPending_ = false;
    if (eventQueue_ != nullptr) {
        vQueueDelete(eventQueue_);
        eventQueue_ = nullptr;
    }
    if (stopped_ != nullptr) {
        vSemaphoreDelete(stopped_);
        stopped_ = nullptr;
    }
    releaseBuffers();
    renderer_.end();
}

void AstraGesture3D::update() {
    if (!active_ || eventQueue_ == nullptr) {
        return;
    }

    GestureEvent event;
    while (xQueueReceive(eventQueue_, &event, 0) == pdTRUE) {
        const astra_gesture::Digit digit =
            astra_gesture::digitFor(event.category, event.score);
        if (!debouncer_.accept(digit, event.timestamp)) {
            continue;
        }
        currentDigit_ = digit;
        digitPending_ = true;
        renderer_.setDigit(digit);
        Serial.printf("gesture=%s score=%.3f digit=%u\n",
                      event.category,
                      event.score,
                      static_cast<unsigned>(digit));
    }
}

void AstraGesture3D::render() {
    if (active_ && currentDigit_ != astra_gesture::Digit::None) {
        renderer_.render();
    }
}

bool AstraGesture3D::isActive() const {
    return active_;
}

bool AstraGesture3D::isAvailable() const {
    return available_ && renderer_.available();
}

AstraGestureDigit AstraGesture3D::currentDigit() const {
    return currentDigit_;
}

bool AstraGesture3D::takeNewDigit(AstraGestureDigit &digit) {
    if (!digitPending_) {
        return false;
    }
    digit = currentDigit_;
    digitPending_ = false;
    return digit != astra_gesture::Digit::None;
}

void AstraGesture3D::taskEntry(void *context) {
    auto *gesture = static_cast<AstraGesture3D *>(context);
    if (gesture != nullptr) {
        gesture->run();
        if (gesture->stopped_ != nullptr) {
            xSemaphoreGive(gesture->stopped_);
        }
    }
    vTaskDelete(nullptr);
}

void AstraGesture3D::run() {
    dl::image::img_t image = {
        .data = rgbBuffer_,
        .width = kFrameWidth,
        .height = kFrameHeight,
        .pix_type = dl::image::DL_IMAGE_PIX_TYPE_RGB888,
    };

    HandDetect *detector = nullptr;
    HandGestureRecognizer *recognizer = nullptr;
    std::list<dl::detect::result_t> trackedHands;
    const char *bestCategory = nullptr;
    float bestScore = 0.0F;
    std::size_t classifyRemaining = 0;
    std::uint32_t modelRetryAt = 0;
    bool modelWarningPrinted = false;

    while (!stopRequested_) {
        if (static_cast<std::int32_t>(millis() - modelRetryAt) < 0) {
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        astra_camera::FrameInfo info;
        if (!camera_.copyJpegFrame(jpegBuffer_, kJpegBufferSize, info,
                                   pdMS_TO_TICKS(100))) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        if (info.width != kFrameWidth || info.height != kFrameHeight ||
            !fmt2rgb888(jpegBuffer_, info.length, PIXFORMAT_JPEG, rgbBuffer_)) {
            vTaskDelay(pdMS_TO_TICKS(2));
            continue;
        }

        if (detector == nullptr && recognizer == nullptr) {
            const std::size_t largestPsramBlock =
                heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
            if (largestPsramBlock < kModelWorkspaceMinimum) {
                if (!modelWarningPrinted) {
                    ESP_LOGW(kLogTag,
                             "waiting for gesture model memory: largest PSRAM block=%u bytes",
                             static_cast<unsigned>(largestPsramBlock));
                    modelWarningPrinted = true;
                }
                modelRetryAt = millis() + 1000;
                continue;
            }

            detector = new (std::nothrow) HandDetect(
                static_cast<HandDetect::model_type_t>(CONFIG_DEFAULT_HAND_DETECT_MODEL), false);
            if (detector == nullptr || !detector->isReady()) {
                ESP_LOGE(kLogTag, "failed to allocate hand detector");
                delete detector;
                detector = nullptr;
                modelRetryAt = millis() + 1000;
                vTaskDelay(pdMS_TO_TICKS(20));
                continue;
            }
            modelWarningPrinted = false;
        }

        if (recognizer == nullptr) {
            auto &detections = detector->run(image);
            if (detections.empty()) {
                vTaskDelay(pdMS_TO_TICKS(2));
                continue;
            }

            trackedHands = detections;
            delete detector;
            detector = nullptr;
            recognizer = new (std::nothrow) HandGestureRecognizer(
                HandGestureCls::MOBILENETV2_0_5_S8_V1);
            if (recognizer == nullptr || !recognizer->isReady()) {
                ESP_LOGE(kLogTag, "failed to allocate hand gesture model");
                delete recognizer;
                recognizer = nullptr;
                trackedHands.clear();
                modelRetryAt = millis() + 1000;
                vTaskDelay(pdMS_TO_TICKS(20));
                continue;
            }
            classifyRemaining = kClassifyBurstFrames;
            bestCategory = nullptr;
            bestScore = 0.0F;
        }

        const std::vector<dl::cls::result_t> results =
            recognizer->recognize(image, trackedHands);
        for (const auto &result : results) {
            if (result.cat_name != nullptr &&
                (bestCategory == nullptr || result.score > bestScore)) {
                bestCategory = result.cat_name;
                bestScore = result.score;
            }
        }

        if (classifyRemaining > 0) {
            --classifyRemaining;
        }
        if (classifyRemaining == 0) {
            if (bestCategory != nullptr && eventQueue_ != nullptr) {
                GestureEvent event;
                std::strncpy(event.category,
                             bestCategory,
                             sizeof(event.category) - 1);
                event.category[sizeof(event.category) - 1] = '\0';
                event.score = bestScore;
                event.timestamp = millis();
                xQueueOverwrite(eventQueue_, &event);
            }

            delete recognizer;
            recognizer = nullptr;
            trackedHands.clear();
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }

    delete recognizer;
    delete detector;
}

void AstraGesture3D::releaseBuffers() {
    if (jpegBuffer_ != nullptr) {
        heap_caps_free(jpegBuffer_);
        jpegBuffer_ = nullptr;
    }
    if (rgbBuffer_ != nullptr) {
        heap_caps_free(rgbBuffer_);
        rgbBuffer_ = nullptr;
    }
}
