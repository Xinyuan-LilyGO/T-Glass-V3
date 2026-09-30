#include "AstraPortSurface.h"

#include <algorithm>

#include "fonts/astra_fonts.h"

AstraPortSurface::AstraPortSurface() {
    // U8g2 only provides a 128-pixel-wide full framebuffer. The logical
    // surface exposes the 126x74 region used by AstraPort.
    u8g2_Setup_sh1107_128x128_f(
        &canvas_, U8G2_R0, u8x8_byte_arduino_hw_spi, u8x8_gpio_and_delay_arduino);
    u8g2_SetFontMode(&canvas_, 1);
    u8g2_SetFontDirection(&canvas_, 0);
    u8g2_SetFont(&canvas_, u8g2_font_wqy12_t_gb2312);
    clear();
}

std::int16_t AstraPortSurface::width() const {
    return kWidth;
}

std::int16_t AstraPortSurface::height() const {
    return kHeight;
}

std::uint8_t AstraPortSurface::bufferTileWidth() const {
    return u8g2_GetBufferTileWidth(const_cast<u8g2_t *>(&canvas_));
}

std::uint8_t AstraPortSurface::bufferTileHeight() const {
    return u8g2_GetBufferTileHeight(const_cast<u8g2_t *>(&canvas_));
}

void AstraPortSurface::clear() {
    u8g2_ClearBuffer(&canvas_);
}

void AstraPortSurface::setDrawColor(std::uint8_t color) {
    u8g2_SetDrawColor(&canvas_, color > 2 ? 1 : color);
}

bool AstraPortSurface::pixel(std::int16_t x, std::int16_t y) const {
    if (x < 0 || x >= kWidth || y < 0 || y >= kHeight) {
        return false;
    }

    const auto *buffer = u8g2_GetBufferPtr(const_cast<u8g2_t *>(&canvas_));
    const std::size_t bufferRowWidth =
        static_cast<std::size_t>(bufferTileWidth()) * 8U;
    const std::size_t byteIndex = static_cast<std::size_t>(y / 8) * bufferRowWidth + x;
    return (buffer[byteIndex] & (1U << (y & 7))) != 0;
}

std::uint8_t *AstraPortSurface::data() {
    return u8g2_GetBufferPtr(&canvas_);
}

const std::uint8_t *AstraPortSurface::data() const {
    return u8g2_GetBufferPtr(const_cast<u8g2_t *>(&canvas_));
}

void AstraPortSurface::drawPixel(std::int16_t x, std::int16_t y) {
    u8g2_DrawPixel(&canvas_, coordinate(x), coordinate(y));
}

void AstraPortSurface::drawBox(std::int16_t x,
                               std::int16_t y,
                               std::int16_t width,
                               std::int16_t height) {
    if (width <= 0 || height <= 0) {
        return;
    }
    u8g2_DrawBox(&canvas_, coordinate(x), coordinate(y), width, height);
}

void AstraPortSurface::drawFrame(std::int16_t x,
                                 std::int16_t y,
                                 std::int16_t width,
                                 std::int16_t height) {
    if (width <= 0 || height <= 0) {
        return;
    }
    u8g2_DrawFrame(&canvas_, coordinate(x), coordinate(y), width, height);
}

void AstraPortSurface::drawRoundedBox(std::int16_t x,
                                      std::int16_t y,
                                      std::int16_t width,
                                      std::int16_t height,
                                      std::int16_t radius) {
    if (width <= 0 || height <= 0 || radius < 0) {
        return;
    }
    u8g2_DrawRBox(&canvas_, coordinate(x), coordinate(y), width, height, radius);
}

void AstraPortSurface::drawRoundedFrame(std::int16_t x,
                                        std::int16_t y,
                                        std::int16_t width,
                                        std::int16_t height,
                                        std::int16_t radius) {
    if (width <= 0 || height <= 0 || radius < 0) {
        return;
    }
    u8g2_DrawRFrame(&canvas_, coordinate(x), coordinate(y), width, height, radius);
}

void AstraPortSurface::drawText(std::int16_t x,
                                std::int16_t baseline,
                                const std::string &text) {
    u8g2_DrawUTF8(&canvas_, coordinate(x), coordinate(baseline), text.c_str());
}

void AstraPortSurface::drawEnglish(std::int16_t x,
                                   std::int16_t baseline,
                                   const std::string &text) {
    u8g2_DrawStr(&canvas_, coordinate(x), coordinate(baseline), text.c_str());
}

void AstraPortSurface::setFont(const std::uint8_t *font) {
    if (font == nullptr) {
        return;
    }
    u8g2_SetFontMode(&canvas_, 1);
    u8g2_SetFontDirection(&canvas_, 0);
    u8g2_SetFont(&canvas_, font);
}

std::uint16_t AstraPortSurface::fontWidth(const std::string &text) {
    return static_cast<std::uint16_t>(u8g2_GetUTF8Width(&canvas_, text.c_str()));
}

std::uint8_t AstraPortSurface::fontHeight() {
    return static_cast<std::uint8_t>(u8g2_GetMaxCharHeight(&canvas_));
}

void AstraPortSurface::drawBitmap(std::int16_t x,
                                  std::int16_t y,
                                  std::int16_t width,
                                  std::int16_t height,
                                  const std::uint8_t *bitmap) {
    if (bitmap == nullptr || width <= 0 || height <= 0) {
        return;
    }
    u8g2_DrawXBMP(&canvas_, coordinate(x), coordinate(y), width, height, bitmap);
}

void AstraPortSurface::toRgb565(std::uint16_t *out,
                                std::uint16_t black,
                                std::uint16_t white) const {
    toRgb565(out, black, white, kPhysicalOffsetX, kPhysicalOffsetY);
}

void AstraPortSurface::toRgb565(std::uint16_t *out,
                                std::uint16_t black,
                                std::uint16_t white,
                                std::int16_t offsetX,
                                std::int16_t offsetY) const {
    if (out == nullptr) {
        return;
    }

    std::fill(out, out + kPhysicalPixelCount, black);
    const auto *buffer = data();
    const std::size_t bufferRowWidth =
        static_cast<std::size_t>(bufferTileWidth()) * 8U;
    for (std::int16_t y = 0; y < kHeight; ++y) {
        const std::size_t tileRow = static_cast<std::size_t>(y / 8) * bufferRowWidth;
        for (std::int16_t x = 0; x < kWidth; ++x) {
            const std::size_t byteIndex = tileRow + x;
            if ((buffer[byteIndex] & (1U << (y & 7))) != 0) {
                const std::int16_t physicalX = x + offsetX;
                const std::int16_t physicalY = y + offsetY;
                if (physicalX >= 0 && physicalX < kPhysicalWidth && physicalY >= 0 &&
                    physicalY < kPhysicalHeight) {
                    out[static_cast<std::size_t>(physicalY) * kPhysicalWidth +
                        static_cast<std::size_t>(physicalX)] = white;
                }
            }
        }
    }
}

u8g2_uint_t AstraPortSurface::coordinate(std::int16_t value) {
    return static_cast<u8g2_uint_t>(value);
}
