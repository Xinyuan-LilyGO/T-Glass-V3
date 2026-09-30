#pragma once

#include <cstdint>

#include <LilyGo_GlassV3.h>

#include "AstraGesturePolicy.h"

class AstraGesture3DRenderer {
public:
    explicit AstraGesture3DRenderer(LilyGo_Glass &glass);
    ~AstraGesture3DRenderer();

    AstraGesture3DRenderer(const AstraGesture3DRenderer &) = delete;
    AstraGesture3DRenderer &operator=(const AstraGesture3DRenderer &) = delete;

    bool begin();
    void end();
    void setDigit(astra_gesture::Digit digit);
    void render();
    bool available() const;

    struct DigitMask {
        std::uint8_t rows[7];
    };

private:
    static const DigitMask &maskFor(astra_gesture::Digit digit);
    bool rebuildObject(astra_gesture::Digit digit);

    LilyGo_Glass &glass_;
    std::uint16_t *frameBuffer_ = nullptr;
    std::uint16_t *depthBuffer_ = nullptr;
    void *scene_ = nullptr;
    void *camera_ = nullptr;
    void *object_ = nullptr;
    astra_gesture::Digit digit_ = astra_gesture::Digit::None;
    std::uint32_t lastFrameMs_ = 0;
    float animationSeconds_ = 0.0F;
};
