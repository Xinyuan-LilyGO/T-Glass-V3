#pragma once

#include <cstdint>
#include <string>

#include <LilyGo_GlassV3.h>

#include "hal.h"
#include "../AstraStatusBar.h"
#include "../AstraPortSurface.h"

enum class AstraScreenPattern : std::uint8_t {
    White,
    Black,
    Red,
    Green,
    Blue,
    Checkerboard,
};

class AstraGlassHal final : public HAL {
public:
    explicit AstraGlassHal(LilyGo_Glass &glass);
    ~AstraGlassHal() override;

    void init() override;

    void *_getCanvasBuffer() override;
    unsigned char _getBufferTileHeight() override;
    unsigned char _getBufferTileWidth() override;
    void _canvasUpdate() override;
    void _canvasClear() override;
    void _setFont(const unsigned char *font) override;
    unsigned char _getFontWidth(std::string &text) override;
    unsigned char _getFontHeight() override;
    void _setDrawType(unsigned char type) override;
    void _drawPixel(float x, float y) override;
    void _drawEnglish(float x, float y, const std::string &text) override;
    void _drawChinese(float x, float y, const std::string &text) override;
    void _drawVDottedLine(float x, float y, float height) override;
    void _drawHDottedLine(float x, float y, float width) override;
    void _drawVLine(float x, float y, float height) override;
    void _drawHLine(float x, float y, float width) override;
    void _drawBMP(float x,
                  float y,
                  float width,
                  float height,
                  const unsigned char *bitmap) override;
    void _drawBox(float x, float y, float width, float height) override;
    void _drawRBox(float x, float y, float width, float height, float radius) override;
    void _drawFrame(float x, float y, float width, float height) override;
    void _drawRFrame(float x, float y, float width, float height, float radius) override;

    void _delay(unsigned long milliseconds) override;
    unsigned long _millis() override;
    unsigned long _getTick() override;
    unsigned long _getRandomSeed() override;
    void _screenOn() override;
    void _screenOff() override;

    void _beep(float frequency) override;
    void _beepStop() override;
    void _setBeepVol(unsigned char volume) override;

    bool _getKey(key::KEY_INDEX keyIndex) override;
    void _updateConfig() override;

    void setDisplayCalibration(std::int16_t offsetX, std::int16_t offsetY);
    void setContentOffsetY(std::int16_t offsetY);
    bool presentCameraFrame(const std::uint8_t *frame,
                            std::size_t frameLength,
                            std::uint16_t width,
                            std::uint16_t height);
    bool presentScreenPattern(AstraScreenPattern pattern);

    AstraPortSurface &surface();

private:
    static constexpr std::uint16_t kDisplayOffsetY = 168;
    static std::int16_t coordinate(float value);
    std::int16_t coordinateY(float value) const;

    LilyGo_Glass &glass_;
    AstraPortSurface surface_;
    std::uint16_t *rgb565_ = nullptr;
    bool cameraFramePending_ = false;
    std::uint8_t restoreBrightness_ = 255;
    std::int16_t displayOffsetX_ = AstraPortSurface::kPhysicalOffsetX;
    std::int16_t displayOffsetY_ = AstraPortSurface::kPhysicalOffsetY;
    std::int16_t contentOffsetY_ = astra_status_bar::kContentTopInset;
};
