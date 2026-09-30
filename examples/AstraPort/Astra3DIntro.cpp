#include "Astra3DIntro.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

#include <Arduino.h>
#include <LilyGo_GlassV3.h>

#include <Jet.hpp>

using namespace Renderer;

namespace {

constexpr uint16_t kScreenWidth = 126;
constexpr uint16_t kScreenHeight = 126;
constexpr uint16_t kDisplayOffsetY = 168;
constexpr uint32_t kPixelCount =
    static_cast<uint32_t>(kScreenWidth) * static_cast<uint32_t>(kScreenHeight);

constexpr uint32_t kIntroDurationMs = 3000;
constexpr uint32_t kFrameIntervalMs = 30;
constexpr uint32_t kRevealDurationMs = 650;
constexpr uint32_t kFadeOutDurationMs = 550;
constexpr uint32_t kFadeOutStartMs = kIntroDurationMs - kFadeOutDurationMs;
constexpr float kPi = 3.14159265358979323846f;

constexpr int32_t kGlyphWidth = 5;
constexpr int32_t kGlyphHeight = 7;
constexpr int32_t kLetterGap = 1;
constexpr int32_t kCellWidth = 17;
constexpr int32_t kCellHeight = 24;
constexpr int32_t kExtrusion = 30;
constexpr int32_t kWordLength = 6;
constexpr int32_t kWordColumns =
    kWordLength * kGlyphWidth + (kWordLength - 1) * kLetterGap;
constexpr int32_t kWordWidth = kWordColumns * kCellWidth;
constexpr int32_t kWordHeight = kGlyphHeight * kCellHeight;
constexpr int32_t kLogoZ = 620;
constexpr int32_t kStartZ = 1050;
constexpr int32_t kExitZ = 860;

constexpr uint16_t rgb565(uint8_t red, uint8_t green, uint8_t blue)
{
    return static_cast<uint16_t>(((red & 0xF8) << 8) |
                                 ((green & 0xFC) << 3) |
                                 (blue >> 3));
}

constexpr uint16_t kColorBackground = rgb565(0, 0, 0);
constexpr uint16_t kColorText = rgb565(255, 255, 255);

struct Glyph {
    char character;
    uint8_t rows[kGlyphHeight];
};

// One bit is one column; bit 0 is the leftmost column.
constexpr Glyph kGlyphs[] = {
    {'L', {0b00001, 0b00001, 0b00001, 0b00001, 0b00001, 0b00001, 0b11111}},
    {'I', {0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b11111}},
    {'Y', {0b10001, 0b10001, 0b01010, 0b00100, 0b00100, 0b00100, 0b00100}},
    {'G', {0b01110, 0b10001, 0b00001, 0b11101, 0b10001, 0b10001, 0b01110}},
    {'O', {0b01110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110}},
};

Material frontMaterial(kColorText, nullptr, nullptr, true);
Material topMaterial(kColorText, nullptr, nullptr, true);
Material sideMaterial(kColorText, nullptr, nullptr, true);
Material backMaterial(kColorText, nullptr, nullptr, true);

float clampUnit(float value)
{
    return std::max(0.0f, std::min(value, 1.0f));
}

float easeOutCubic(float value)
{
    const float inverse = 1.0f - clampUnit(value);
    return 1.0f - inverse * inverse * inverse;
}

float easeInCubic(float value)
{
    const float unit = clampUnit(value);
    return unit * unit * unit;
}

int32_t interpolate(int32_t from, int32_t to, float amount)
{
    return from + static_cast<int32_t>(std::lround(
        static_cast<float>(to - from) * clampUnit(amount)));
}

int32_t wrapAngle(int32_t angle)
{
    angle %= ANGLE_MAX;
    return angle < 0 ? angle + ANGLE_MAX : angle;
}

Object::Vertex makeVertex(int32_t x, int32_t y, int32_t z)
{
    Object::Vertex vertex;
    vertex.position.assign(x, y, z);
    vertex.uv.assign(0, 0);
    vertex.normal.assign(0, 0, 0);
    return vertex;
}

void addCuboid(Object &object,
               int32_t x0,
               int32_t y0,
               int32_t x1,
               int32_t y1)
{
    const int32_t halfDepth = kExtrusion / 2;
    const uint16_t base = static_cast<uint16_t>(object.vertices.size());

    object.addVertex(makeVertex(x0, y0, halfDepth));
    object.addVertex(makeVertex(x1, y0, halfDepth));
    object.addVertex(makeVertex(x1, y1, halfDepth));
    object.addVertex(makeVertex(x0, y1, halfDepth));

    object.addVertex(makeVertex(x1, y0, -halfDepth));
    object.addVertex(makeVertex(x0, y0, -halfDepth));
    object.addVertex(makeVertex(x0, y1, -halfDepth));
    object.addVertex(makeVertex(x1, y1, -halfDepth));

    object.addVertex(makeVertex(x0, y0, -halfDepth));
    object.addVertex(makeVertex(x0, y0, halfDepth));
    object.addVertex(makeVertex(x0, y1, halfDepth));
    object.addVertex(makeVertex(x0, y1, -halfDepth));

    object.addVertex(makeVertex(x1, y0, halfDepth));
    object.addVertex(makeVertex(x1, y0, -halfDepth));
    object.addVertex(makeVertex(x1, y1, -halfDepth));
    object.addVertex(makeVertex(x1, y1, halfDepth));

    object.addVertex(makeVertex(x0, y1, halfDepth));
    object.addVertex(makeVertex(x1, y1, halfDepth));
    object.addVertex(makeVertex(x1, y1, -halfDepth));
    object.addVertex(makeVertex(x0, y1, -halfDepth));

    object.addVertex(makeVertex(x0, y0, -halfDepth));
    object.addVertex(makeVertex(x1, y0, -halfDepth));
    object.addVertex(makeVertex(x1, y0, halfDepth));
    object.addVertex(makeVertex(x0, y0, halfDepth));

    object.addFace(base + 0, base + 1, base + 2, base + 3, &frontMaterial);
    object.addFace(base + 4, base + 5, base + 6, base + 7, &backMaterial);
    object.addFace(base + 8, base + 9, base + 10, base + 11, &sideMaterial);
    object.addFace(base + 12, base + 13, base + 14, base + 15, &sideMaterial);
    object.addFace(base + 16, base + 17, base + 18, base + 19, &topMaterial);
    object.addFace(base + 20, base + 21, base + 22, base + 23, &sideMaterial);
}

const Glyph *findGlyph(char character)
{
    for (const Glyph &glyph : kGlyphs) {
        if (glyph.character == character) {
            return &glyph;
        }
    }
    return nullptr;
}

Object *buildLilyGoLogo()
{
    Object *object = new Object();
    const char word[] = "LILYGO";
    const int32_t left = -kWordWidth / 2;
    const int32_t top = kWordHeight / 2;

    for (int32_t letterIndex = 0; letterIndex < kWordLength; ++letterIndex) {
        const Glyph *glyph = findGlyph(word[letterIndex]);
        if (glyph == nullptr) {
            continue;
        }

        const int32_t letterLeft =
            left + letterIndex * (kGlyphWidth + kLetterGap) * kCellWidth;
        for (int32_t row = 0; row < kGlyphHeight; ++row) {
            const uint8_t rowMask = glyph->rows[row];
            int32_t column = 0;
            while (column < kGlyphWidth) {
                while (column < kGlyphWidth &&
                       (rowMask & (1U << column)) == 0) {
                    ++column;
                }
                const int32_t runStart = column;
                while (column < kGlyphWidth &&
                       (rowMask & (1U << column)) != 0) {
                    ++column;
                }
                if (runStart == column) {
                    continue;
                }

                const int32_t y1 = top - row * kCellHeight;
                const int32_t y0 = y1 - kCellHeight;
                const int32_t x0 = letterLeft + runStart * kCellWidth;
                const int32_t x1 = letterLeft + column * kCellWidth;
                addCuboid(*object, x0, y0, x1, y1);
            }
        }
    }

    object->calculateBoundingBox();
    object->cullingMode = CullingMode::NO_CULLING;
    object->cachePositions();
    return object;
}

bool allocateBuffers(uint16_t *&frameBuffer, uint16_t *&depthBuffer)
{
    const size_t colorBytes = static_cast<size_t>(kPixelCount) * sizeof(uint16_t);
    const size_t depthCount =
        static_cast<size_t>(ZBUFFER_STRIDE(kScreenWidth)) * kScreenHeight;
    const size_t depthBytes = depthCount * sizeof(uint16_t);

    frameBuffer = static_cast<uint16_t *>(ps_malloc(colorBytes));
    if (frameBuffer == nullptr) {
        frameBuffer = static_cast<uint16_t *>(malloc(colorBytes));
    }

    depthBuffer = static_cast<uint16_t *>(ps_malloc(depthBytes));
    if (depthBuffer == nullptr) {
        depthBuffer = static_cast<uint16_t *>(malloc(depthBytes));
    }

    if (frameBuffer == nullptr || depthBuffer == nullptr) {
        free(frameBuffer);
        free(depthBuffer);
        frameBuffer = nullptr;
        depthBuffer = nullptr;
        return false;
    }
    return true;
}

void setMaterialAlpha(uint8_t alpha)
{
    frontMaterial.alpha = alpha;
    topMaterial.alpha = alpha;
    sideMaterial.alpha = alpha;
    backMaterial.alpha = alpha;
}

void updateAnimation(Object &logo, uint32_t elapsedMs)
{
    if (elapsedMs < kRevealDurationMs) {
        const float progress = easeOutCubic(
            static_cast<float>(elapsedMs) / kRevealDurationMs);
        logo.setPosition(0, 0, interpolate(kStartZ, kLogoZ, progress));
        logo.setRotation(interpolate(0, 8, progress),
                         wrapAngle(interpolate(300, 345, progress)),
                         0);
        setMaterialAlpha(255);
        return;
    }

    if (elapsedMs < kFadeOutStartMs) {
        const float progress = static_cast<float>(elapsedMs - kRevealDurationMs) /
                               static_cast<float>(kFadeOutStartMs - kRevealDurationMs);
        const float wave = std::sin(progress * 2.0f * kPi);
        logo.setPosition(0, 0, kLogoZ);
        logo.setRotation(8 + static_cast<int32_t>(wave * 2.0f),
                         wrapAngle(350 + static_cast<int32_t>(wave * 12.0f)),
                         static_cast<int32_t>(wave * 2.0f));
        setMaterialAlpha(255);
        return;
    }

    const float progress = easeInCubic(
        static_cast<float>(elapsedMs - kFadeOutStartMs) / kFadeOutDurationMs);
    logo.setPosition(0, 0, interpolate(kLogoZ, kExitZ, progress));
    logo.setRotation(interpolate(8, 18, progress),
                     wrapAngle(interpolate(350, 35, progress)),
                     interpolate(2, 0, progress));
    setMaterialAlpha(static_cast<uint8_t>(std::lround(255.0f * (1.0f - progress))));
}

void flushFrame(LilyGo_Glass &glass, uint16_t *frameBuffer)
{
    glass.setAddrWindow(0,
                        kDisplayOffsetY,
                        kScreenWidth,
                        kScreenHeight);
    glass.pushColors(frameBuffer, kPixelCount);
}

} // namespace

namespace astra {

void run3DIntro(LilyGo_Glass &glass)
{
    uint16_t *frameBuffer = nullptr;
    uint16_t *depthBuffer = nullptr;
    if (!allocateBuffers(frameBuffer, depthBuffer)) {
        return;
    }

    Object *logo = buildLilyGoLogo();
    if (logo == nullptr) {
        free(frameBuffer);
        free(depthBuffer);
        return;
    }

    {
        Scene scene(frameBuffer, depthBuffer, kScreenWidth, kScreenHeight);
        Camera camera;

        scene.setBackcolor(kColorBackground);
        scene.setClearBuffer(true);
        camera.setPosition(0, 0, 0);
        camera.setFOV(64.0f, kScreenWidth);
        camera.nearPlane = 64;
        camera.farPlane = 1800;
        scene.setCamera(&camera);

        scene.addObject(logo);

        const uint32_t startMs = millis();
        uint32_t nextFrameMs = 0;
        while (true) {
            const uint32_t elapsedMs = millis() - startMs;
            if (elapsedMs >= kIntroDurationMs) {
                break;
            }
            if (elapsedMs < nextFrameMs) {
                delay(1);
                continue;
            }

            updateAnimation(*logo, elapsedMs);
            scene.render();
            flushFrame(glass, frameBuffer);
            nextFrameMs += kFrameIntervalMs;
        }
    }

    std::fill(frameBuffer, frameBuffer + kPixelCount, kColorBackground);
    flushFrame(glass, frameBuffer);
    delete logo;
    free(frameBuffer);
    free(depthBuffer);
}

} // namespace astra
