#include "DinoJumpRenderer.h"

#include <Arduino.h>
#include <pgmspace.h>

#include <cstdio>

#include "../AstraPortSurface.h"
#include "../fonts/astra_fonts.h"
#include "imgData.h"

namespace {

constexpr std::uint8_t kDinoSourceWidth = 33;
constexpr std::uint8_t kDinoSourceHeight = 35;
constexpr std::uint8_t kEnemySourceWidth = 18;
constexpr std::uint8_t kEnemySourceHeight = 38;
constexpr std::uint8_t kCloudSourceWidth = 38;
constexpr std::uint8_t kCloudSourceHeight = 11;
constexpr std::uint8_t kBumpSourceWidth = 34;
constexpr std::uint8_t kBumpSourceHeight = 5;
constexpr std::uint8_t kCloudWidth = 26;
constexpr std::uint8_t kCloudHeight = 8;
constexpr std::uint8_t kBumpWidth = 25;
constexpr std::uint8_t kBumpHeight = 4;
constexpr std::int16_t kPlayerX = 16;
constexpr std::int16_t kCloudY = 14;
constexpr std::int16_t kBumpY = 48;
constexpr std::int16_t kScoreX = 103;

std::int16_t roundPosition(float value) {
    return static_cast<std::int16_t>(value + 0.5f);
}

void setGameFont(AstraPortSurface &surface) {
    surface.setFont(u8g2_font_wqy12_t_gb2312);
    surface.setDrawColor(1);
}

void drawCentered(AstraPortSurface &surface, const char *text, std::int16_t baseline) {
    const std::string value(text);
    const std::int16_t x = static_cast<std::int16_t>(
        (AstraPortSurface::kWidth - surface.fontWidth(value)) / 2);
    surface.drawEnglish(x, baseline, value);
}

} // namespace

void DinoJumpRenderer::render(AstraPortSurface &surface, const DinoJumpView &view) const {
    surface.clear();
    setGameFont(surface);
    drawClouds(surface, view);
    drawGround(surface, view);

    if (view.state == DinoJumpState::Menu) {
        drawMenu(surface);
        return;
    }

    drawObstacles(surface, view);
    drawDino(surface,
             kPlayerX,
             static_cast<std::int16_t>(view.playerY + 0.5f),
             view.animationFrame);
    drawScore(surface, view);
    if (view.state == DinoJumpState::GameOver) {
        drawGameOver(surface, view);
    }
}

void DinoJumpRenderer::drawBitmapScaled(AstraPortSurface &surface,
                                         std::int16_t x,
                                         std::int16_t y,
                                         const unsigned char *bitmap,
                                         std::uint8_t sourceWidth,
                                         std::uint8_t sourceHeight,
                                         std::uint8_t width,
                                         std::uint8_t height) {
    if (bitmap == nullptr || sourceWidth == 0 || sourceHeight == 0 || width == 0 ||
        height == 0) {
        return;
    }

    const std::uint8_t bytesPerRow = static_cast<std::uint8_t>((sourceWidth + 7) / 8);
    for (std::uint8_t row = 0; row < height; ++row) {
        const std::uint8_t sourceRow = static_cast<std::uint8_t>(
            (static_cast<std::uint16_t>(row) * sourceHeight) / height);
        for (std::uint8_t column = 0; column < width; ++column) {
            const std::uint8_t sourceColumn = static_cast<std::uint8_t>(
                (static_cast<std::uint16_t>(column) * sourceWidth) / width);
            const std::uint8_t data = pgm_read_byte(
                bitmap + sourceRow * bytesPerRow + sourceColumn / 8);
            if ((data & (1U << (sourceColumn & 7))) != 0) {
                surface.drawPixel(static_cast<std::int16_t>(x + column),
                                  static_cast<std::int16_t>(y + row));
            }
        }
    }
}

void DinoJumpRenderer::drawDino(AstraPortSurface &surface,
                                std::int16_t x,
                                std::int16_t y,
                                std::uint8_t frame) {
    const std::uint8_t safeFrame = static_cast<std::uint8_t>(frame % 3);
    drawBitmapScaled(surface,
                     x,
                     y,
                     dino[safeFrame],
                     kDinoSourceWidth,
                     kDinoSourceHeight,
                     DinoJumpGame::kDinoRenderWidth,
                     DinoJumpGame::kDinoRenderHeight);
}

void DinoJumpRenderer::drawClouds(AstraPortSurface &surface, const DinoJumpView &view) {
    for (std::uint8_t index = 0; index < 2; ++index) {
        drawBitmapScaled(surface,
                         roundPosition(view.cloudX[index]),
                         kCloudY,
                         cloud,
                         kCloudSourceWidth,
                         kCloudSourceHeight,
                         kCloudWidth,
                         kCloudHeight);
    }
}

void DinoJumpRenderer::drawGround(AstraPortSurface &surface, const DinoJumpView &view) {
    surface.drawBox(0, DinoJumpGame::kGroundLine, AstraPortSurface::kWidth, 1);

    for (std::uint8_t index = 0; index < 6; ++index) {
        surface.drawBox(roundPosition(view.groundMarkX[index]),
                        DinoJumpGame::kGroundLine + 8,
                        view.groundMarkWidth[index],
                        1);
        surface.drawBox(roundPosition(view.groundMark2X[index]),
                        DinoJumpGame::kGroundLine + 10,
                        view.groundMark2Width[index],
                        1);
    }

    for (std::uint8_t index = 0; index < 2; ++index) {
        drawBitmapScaled(surface,
                         roundPosition(view.bumpX[index]),
                         kBumpY,
                         bump[view.bumpFrame[index] % 2],
                         kBumpSourceWidth,
                         kBumpSourceHeight,
                         kBumpWidth,
                         kBumpHeight);
    }
}

void DinoJumpRenderer::drawObstacles(AstraPortSurface &surface, const DinoJumpView &view) {
    for (std::uint8_t index = 0; index < 2; ++index) {
        drawBitmapScaled(surface, roundPosition(view.obstacleX[index]), DinoJumpGame::kObstacleY, enemy[index], kEnemySourceWidth, kEnemySourceHeight, DinoJumpGame::kEnemyRenderWidth, DinoJumpGame::kEnemyRenderHeight);
    }
}

void DinoJumpRenderer::drawScore(AstraPortSurface &surface, const DinoJumpView &view) {
    char scoreText[6] = {};
    std::snprintf(scoreText, sizeof(scoreText), "%04u", static_cast<unsigned>(view.score));
    surface.drawEnglish(kScoreX, 8, scoreText);
}

void DinoJumpRenderer::drawMenu(AstraPortSurface &surface) {
    drawCentered(surface, "DINO RUN", 9);
    drawDino(surface, 54, 18, 1);
    drawCentered(surface, "TOUCH START", 53);
    drawCentered(surface, "BOOT JUMP", 63);
}

void DinoJumpRenderer::drawGameOver(AstraPortSurface &surface, const DinoJumpView &view) {
    surface.drawFrame(22, 17, 84, 31);
    drawCentered(surface, "GAME OVER", 28);

    char scoreText[16] = {};
    std::snprintf(scoreText, sizeof(scoreText), "S:%04u B:%04u",
                  static_cast<unsigned>(view.score),
                  static_cast<unsigned>(view.bestScore));
    drawCentered(surface, scoreText, 40);
    drawCentered(surface, "TOUCH RETRY", 57);
}
