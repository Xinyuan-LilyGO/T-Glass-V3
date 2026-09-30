#pragma once

#include <cstdint>
#include <string>

#include <U8g2lib.h>

class AstraPortSurface {
public:
    static constexpr std::int16_t kWidth = 126;
    static constexpr std::int16_t kHeight = 74;
    static constexpr std::int16_t kContentWidth = 126;
    static constexpr std::int16_t kContentHeight = 64;
    static constexpr std::int16_t kPhysicalWidth = 126;
    static constexpr std::int16_t kPhysicalHeight = 126;
    static constexpr std::int16_t kPhysicalOffsetX = 0;
    static constexpr std::int16_t kPhysicalOffsetY =
        (kPhysicalHeight - kHeight) / 2;
    static constexpr std::size_t kPhysicalPixelCount =
        static_cast<std::size_t>(kPhysicalWidth) * kPhysicalHeight;

    AstraPortSurface();
    AstraPortSurface(const AstraPortSurface &) = delete;
    AstraPortSurface &operator=(const AstraPortSurface &) = delete;

    std::int16_t width() const;
    std::int16_t height() const;
    std::uint8_t bufferTileWidth() const;
    std::uint8_t bufferTileHeight() const;

    void clear();
    void setDrawColor(std::uint8_t color);
    bool pixel(std::int16_t x, std::int16_t y) const;
    std::uint8_t *data();
    const std::uint8_t *data() const;

    void drawPixel(std::int16_t x, std::int16_t y);
    void drawBox(std::int16_t x, std::int16_t y, std::int16_t width, std::int16_t height);
    void drawFrame(std::int16_t x, std::int16_t y, std::int16_t width, std::int16_t height);
    void drawRoundedBox(std::int16_t x,
                        std::int16_t y,
                        std::int16_t width,
                        std::int16_t height,
                        std::int16_t radius);
    void drawRoundedFrame(std::int16_t x,
                          std::int16_t y,
                          std::int16_t width,
                          std::int16_t height,
                          std::int16_t radius);
    void drawText(std::int16_t x, std::int16_t baseline, const std::string &text);
    void drawEnglish(std::int16_t x, std::int16_t baseline, const std::string &text);
    void setFont(const std::uint8_t *font);
    std::uint16_t fontWidth(const std::string &text);
    std::uint8_t fontHeight();
    void drawBitmap(std::int16_t x,
                    std::int16_t y,
                    std::int16_t width,
                    std::int16_t height,
                    const std::uint8_t *bitmap);

    void toRgb565(std::uint16_t *out, std::uint16_t black, std::uint16_t white) const;
    void toRgb565(std::uint16_t *out,
                  std::uint16_t black,
                  std::uint16_t white,
                  std::int16_t offsetX,
                  std::int16_t offsetY) const;

private:
    static u8g2_uint_t coordinate(std::int16_t value);

    u8g2_t canvas_{};
};
