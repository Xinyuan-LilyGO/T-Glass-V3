#include <Arduino.h>
#include <LilyGo_GlassV3.h>

#include <Jet.hpp>

#include <math.h>
#include <stdlib.h>

using namespace Renderer;

namespace {

constexpr uint16_t kScreenWidth = 126;
constexpr uint16_t kScreenHeight = 126;
constexpr uint16_t kDisplayOffsetY = 168;
constexpr uint32_t kPixelCount =
    static_cast<uint32_t>(kScreenWidth) * static_cast<uint32_t>(kScreenHeight);
constexpr uint32_t kFrameIntervalMs = 30;

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

uint16_t *frameBuffer = nullptr;
uint16_t *depthBuffer = nullptr;
Scene *scene = nullptr;
Object *logo = nullptr;
Camera camera;
uint32_t lastFrameAt = 0;
float animationSeconds = 0.0f;
float yawDegrees = 0.0f;

Material frontMaterial(kColorText, nullptr, nullptr, true);
Material topMaterial(kColorText, nullptr, nullptr, true);
Material sideMaterial(kColorText, nullptr, nullptr, true);
Material backMaterial(kColorText, nullptr, nullptr, true);

[[noreturn]] void stopWithMessage(const char *message)
{
    while (true) {
        Serial.println(message);
        delay(1000);
    }
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

    // The winding matches Jet's Primitives::createCube(): +Z is the front.
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

bool allocateBuffers()
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

void flushFrame()
{
    glass.setAddrWindow(0, kDisplayOffsetY, kScreenWidth, kScreenHeight);
    glass.pushColors(frameBuffer, kPixelCount);
}

void renderFrame()
{
    if (scene == nullptr || logo == nullptr) {
        return;
    }

    scene->render();
    flushFrame();
}

void updateAnimation(float deltaSeconds)
{
    animationSeconds += deltaSeconds;
    yawDegrees += deltaSeconds * 38.0f;
    if (yawDegrees >= 360.0f) {
        yawDegrees -= 360.0f;
    }

    const int32_t pitch = 8 + static_cast<int32_t>(sin(animationSeconds * 1.4f) * 5.0f);
    const int32_t yaw = static_cast<int32_t>(yawDegrees);
    const int32_t roll = static_cast<int32_t>(sin(animationSeconds * 0.8f) * 3.0f);
    logo->setRotation(pitch, yaw, roll);
}

} // namespace

void setup()
{
    Serial.begin(115200);

    if (!glass.begin()) {
        stopWithMessage("T-Glass V3 initialization failed");
    }
    glass.setBrightness(255);

    if (!allocateBuffers()) {
        stopWithMessage("LilyGo3D framebuffer allocation failed");
    }

    scene = new Scene(frameBuffer, depthBuffer, kScreenWidth, kScreenHeight);
    scene->setBackcolor(kColorBackground);
    scene->setClearBuffer(true);

    camera.setPosition(0, 0, 0);
    camera.setFOV(64.0f, kScreenWidth);
    camera.nearPlane = 64;
    camera.farPlane = 1600;
    scene->setCamera(&camera);

    logo = buildLilyGoLogo();
    logo->setPosition(0, 0, kLogoZ);
    scene->addObject(logo);

    lastFrameAt = millis();
    renderFrame();
}

void loop()
{
    const uint32_t now = millis();
    if (now - lastFrameAt < kFrameIntervalMs) {
        delay(1);
        return;
    }

    uint32_t elapsed = now - lastFrameAt;
    lastFrameAt = now;
    if (elapsed > 100) {
        elapsed = 100;
    }

    updateAnimation(elapsed / 1000.0f);
    renderFrame();
}
