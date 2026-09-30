#pragma once

#include "DinoJumpGame.h"

class AstraPortSurface;

class DinoJumpRenderer {
public:
    void render(AstraPortSurface &surface, const DinoJumpView &view) const;

private:
    static void drawBitmapScaled(AstraPortSurface &surface,
                                 std::int16_t x,
                                 std::int16_t y,
                                 const unsigned char *bitmap,
                                 std::uint8_t sourceWidth,
                                 std::uint8_t sourceHeight,
                                 std::uint8_t width,
                                 std::uint8_t height);
    static void drawDino(AstraPortSurface &surface,
                         std::int16_t x,
                         std::int16_t y,
                         std::uint8_t frame);
    static void drawClouds(AstraPortSurface &surface, const DinoJumpView &view);
    static void drawGround(AstraPortSurface &surface, const DinoJumpView &view);
    static void drawObstacles(AstraPortSurface &surface, const DinoJumpView &view);
    static void drawScore(AstraPortSurface &surface, const DinoJumpView &view);
    static void drawMenu(AstraPortSurface &surface);
    static void drawGameOver(AstraPortSurface &surface, const DinoJumpView &view);
};
