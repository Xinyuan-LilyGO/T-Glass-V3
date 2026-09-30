#include "AstraGlassHal.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

#include <esp_system.h>

#include "../AstraCameraPreview.h"

AstraGlassHal::AstraGlassHal(LilyGo_Glass &glass) : glass_(glass) {
}

AstraGlassHal::~AstraGlassHal() {
    if (rgb565_ != nullptr) {
        free(rgb565_);
        rgb565_ = nullptr;
    }
}

void AstraGlassHal::init() {
    config.screenWeight = static_cast<unsigned char>(AstraPortSurface::kContentWidth);
    config.screenHeight = static_cast<unsigned char>(AstraPortSurface::kContentHeight);
    config.screenBright = glass_.getBrightness();
    restoreBrightness_ = glass_.getBrightness();

    pinMode(BOARD_BOOT_PIN, INPUT_PULLUP);
    pinMode(BOARD_TOUCH_BUTTON, INPUT);

    const std::size_t pixelCount = AstraPortSurface::kPhysicalPixelCount;
    rgb565_ = static_cast<std::uint16_t *>(ps_malloc(pixelCount * sizeof(std::uint16_t)));
    if (rgb565_ == nullptr) {
        rgb565_ = static_cast<std::uint16_t *>(malloc(pixelCount * sizeof(std::uint16_t)));
    }
    surface_.clear();
}

void *AstraGlassHal::_getCanvasBuffer() {
    return surface_.data();
}

unsigned char AstraGlassHal::_getBufferTileHeight() {
    return surface_.bufferTileHeight();
}

unsigned char AstraGlassHal::_getBufferTileWidth() {
    return surface_.bufferTileWidth();
}

void AstraGlassHal::_canvasUpdate() {
    if (rgb565_ == nullptr) {
        return;
    }

    if (cameraFramePending_) {
        glass_.setAddrWindow(0,
                             kDisplayOffsetY,
                             static_cast<std::uint16_t>(AstraPortSurface::kPhysicalWidth),
                             static_cast<std::uint16_t>(AstraPortSurface::kPhysicalHeight));
        glass_.pushColors(rgb565_,
                          static_cast<std::uint32_t>(AstraPortSurface::kPhysicalPixelCount));
        cameraFramePending_ = false;
        return;
    }

    surface_.toRgb565(rgb565_, 0x0000, 0xFFFF, displayOffsetX_, displayOffsetY_);
    glass_.setAddrWindow(0,
                         kDisplayOffsetY,
                         static_cast<std::uint16_t>(AstraPortSurface::kPhysicalWidth),
                         static_cast<std::uint16_t>(AstraPortSurface::kPhysicalHeight));
    glass_.pushColors(rgb565_,
                      static_cast<std::uint32_t>(AstraPortSurface::kPhysicalPixelCount));
}

void AstraGlassHal::_canvasClear() {
    surface_.clear();
}

void AstraGlassHal::_setFont(const unsigned char *font) {
    surface_.setFont(font);
}

unsigned char AstraGlassHal::_getFontWidth(std::string &text) {
    return static_cast<unsigned char>(std::min<std::uint16_t>(surface_.fontWidth(text), 255));
}

unsigned char AstraGlassHal::_getFontHeight() {
    return surface_.fontHeight();
}

void AstraGlassHal::_setDrawType(unsigned char type) {
    surface_.setDrawColor(type);
}

void AstraGlassHal::_drawPixel(float x, float y) {
    surface_.drawPixel(coordinate(x), coordinateY(y));
}

void AstraGlassHal::_drawEnglish(float x, float y, const std::string &text) {
    surface_.drawEnglish(coordinate(x), coordinateY(y), text);
}

void AstraGlassHal::_drawChinese(float x, float y, const std::string &text) {
    surface_.drawText(coordinate(x), coordinateY(y), text);
}

void AstraGlassHal::_drawVDottedLine(float x, float y, float height) {
    const std::int16_t startX = coordinate(x);
    const std::int16_t startY = coordinateY(y);
    const std::int16_t length = coordinate(height);
    for (std::int16_t index = 0; index < length; ++index) {
        if (index % 8 > 2) {
            surface_.drawPixel(startX, startY + index);
        }
    }
}

void AstraGlassHal::_drawHDottedLine(float x, float y, float width) {
    const std::int16_t startX = coordinate(x);
    const std::int16_t startY = coordinateY(y);
    const std::int16_t length = coordinate(width);
    for (std::int16_t index = 0; index < length; ++index) {
        if (index % 8 > 2) {
            surface_.drawPixel(startX + index, startY);
        }
    }
}

void AstraGlassHal::_drawVLine(float x, float y, float height) {
    surface_.drawBox(coordinate(x), coordinateY(y), 1, coordinate(height));
}

void AstraGlassHal::_drawHLine(float x, float y, float width) {
    surface_.drawBox(coordinate(x), coordinateY(y), coordinate(width), 1);
}

void AstraGlassHal::_drawBMP(float x,
                             float y,
                             float width,
                             float height,
                             const unsigned char *bitmap) {
    surface_.drawBitmap(coordinate(x),
                        coordinateY(y),
                        coordinate(width),
                        coordinate(height),
                        bitmap);
}

void AstraGlassHal::_drawBox(float x, float y, float width, float height) {
    surface_.drawBox(coordinate(x),
                     coordinateY(y),
                     coordinate(width),
                     coordinate(height));
}

void AstraGlassHal::_drawRBox(float x, float y, float width, float height, float radius) {
    surface_.drawRoundedBox(coordinate(x),
                            coordinateY(y),
                            coordinate(width),
                            coordinate(height),
                            coordinate(radius));
}

void AstraGlassHal::_drawFrame(float x, float y, float width, float height) {
    surface_.drawFrame(coordinate(x),
                       coordinateY(y),
                       coordinate(width),
                       coordinate(height));
}

void AstraGlassHal::_drawRFrame(float x, float y, float width, float height, float radius) {
    surface_.drawRoundedFrame(coordinate(x),
                              coordinateY(y),
                              coordinate(width),
                              coordinate(height),
                              coordinate(radius));
}

void AstraGlassHal::_delay(unsigned long milliseconds) {
    ::delay(milliseconds);
}

unsigned long AstraGlassHal::_millis() {
    return ::millis();
}

unsigned long AstraGlassHal::_getTick() {
    return micros();
}

unsigned long AstraGlassHal::_getRandomSeed() {
    return static_cast<unsigned long>(esp_random());
}

void AstraGlassHal::_screenOn() {
    if (restoreBrightness_ == 0) {
        restoreBrightness_ = 255;
    }
    glass_.setBrightness(restoreBrightness_);
}

void AstraGlassHal::_screenOff() {
    restoreBrightness_ = glass_.getBrightness();
    glass_.setBrightness(0);
}

void AstraGlassHal::_beep(float frequency) {
    (void)frequency;
}

void AstraGlassHal::_beepStop() {
}

void AstraGlassHal::_setBeepVol(unsigned char volume) {
    (void)volume;
}

bool AstraGlassHal::_getKey(key::KEY_INDEX keyIndex) {
    if (keyIndex == key::KEY_0) {
        return digitalRead(BOARD_BOOT_PIN) == LOW;
    }
    if (keyIndex == key::KEY_1) {
        return digitalRead(BOARD_TOUCH_BUTTON) == HIGH;
    }
    return false;
}

void AstraGlassHal::_updateConfig() {
}

void AstraGlassHal::setDisplayCalibration(std::int16_t offsetX, std::int16_t offsetY) {
    displayOffsetX_ = offsetX;
    displayOffsetY_ = AstraPortSurface::kPhysicalOffsetY + offsetY;
}

void AstraGlassHal::setContentOffsetY(std::int16_t offsetY) {
    contentOffsetY_ = offsetY;
}

bool AstraGlassHal::presentCameraFrame(const std::uint8_t *frame,
                                       std::size_t frameLength,
                                       std::uint16_t width,
                                       std::uint16_t height) {
    cameraFramePending_ = false;
    if (!astra_camera::copyCenteredRgb565Frame(
            rgb565_,
            static_cast<std::uint16_t>(AstraPortSurface::kPhysicalWidth),
            static_cast<std::uint16_t>(AstraPortSurface::kPhysicalHeight),
            frame,
            frameLength,
            width,
            height)) {
        return false;
    }

    cameraFramePending_ = true;
    return true;
}

AstraPortSurface &AstraGlassHal::surface() {
    return surface_;
}

std::int16_t AstraGlassHal::coordinate(float value) {
    return static_cast<std::int16_t>(std::lround(value));
}

std::int16_t AstraGlassHal::coordinateY(float value) const {
    return coordinate(value + static_cast<float>(contentOffsetY_));
}
