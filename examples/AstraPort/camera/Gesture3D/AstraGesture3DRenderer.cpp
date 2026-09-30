#include "AstraGesture3DRenderer.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

#include <Arduino.h>
#include <Jet.hpp>

using namespace Renderer;

namespace {

constexpr std::uint16_t kScreenWidth = 126;
constexpr std::uint16_t kScreenHeight = 126;
constexpr std::uint16_t kDisplayOffsetY = 168;
constexpr std::uint32_t kPixelCount =
    static_cast<std::uint32_t>(kScreenWidth) * kScreenHeight;
constexpr std::uint32_t kFrameIntervalMs = 30;
constexpr float kPi = 3.14159265358979323846F;
constexpr int kGlyphWidth = 5;
constexpr int kGlyphHeight = 7;
constexpr int kCellWidth = 30;
constexpr int kCellHeight = 30;
constexpr int kExtrusion = 36;
constexpr int kDigitZ = 620;

constexpr std::uint16_t rgb565(std::uint8_t red,
                                std::uint8_t green,
                                std::uint8_t blue) {
    return static_cast<std::uint16_t>(((red & 0xF8U) << 8) |
                                      ((green & 0xFCU) << 3) |
                                      (blue >> 3));
}

constexpr std::uint16_t kBackground = rgb565(0, 0, 0);
constexpr std::uint16_t kFront = rgb565(255, 255, 255);
constexpr std::uint16_t kTop = kFront;
constexpr std::uint16_t kSide = kFront;

Material frontMaterial(kFront, nullptr, nullptr, true);
Material topMaterial(kTop, nullptr, nullptr, true);
Material sideMaterial(kSide, nullptr, nullptr, true);

struct Vertex {
    Object::Vertex value;
};

Object::Vertex makeVertex(int x, int y, int z) {
    Object::Vertex vertex;
    vertex.position.assign(x, y, z);
    vertex.uv.assign(0, 0);
    vertex.normal.assign(0, 0, 0);
    return vertex;
}

void addCuboid(Object &object, int x0, int y0, int x1, int y1) {
    const int halfDepth = kExtrusion / 2;
    const std::uint16_t base = static_cast<std::uint16_t>(object.vertices.size());

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
    object.addFace(base + 4, base + 5, base + 6, base + 7, &sideMaterial);
    object.addFace(base + 8, base + 9, base + 10, base + 11, &sideMaterial);
    object.addFace(base + 12, base + 13, base + 14, base + 15, &sideMaterial);
    object.addFace(base + 16, base + 17, base + 18, base + 19, &topMaterial);
    object.addFace(base + 20, base + 21, base + 22, base + 23, &sideMaterial);
}

Object *buildDigit(const AstraGesture3DRenderer::DigitMask &mask) {
    auto *object = new Object();
    const int width = kGlyphWidth * kCellWidth;
    const int height = kGlyphHeight * kCellHeight;
    const int left = -width / 2;
    const int top = height / 2;

    for (int row = 0; row < kGlyphHeight; ++row) {
        int column = 0;
        while (column < kGlyphWidth) {
            while (column < kGlyphWidth &&
                   (mask.rows[row] & (1U << column)) == 0) {
                ++column;
            }
            const int runStart = column;
            while (column < kGlyphWidth &&
                   (mask.rows[row] & (1U << column)) != 0) {
                ++column;
            }
            if (runStart == column) {
                continue;
            }

            const int y1 = top - row * kCellHeight;
            const int y0 = y1 - kCellHeight;
            // The mask literals are written with the most significant bit on
            // the left. Map bit zero to the rightmost cell so the displayed
            // digit is not horizontally mirrored.
            const int x0 = left + (kGlyphWidth - column) * kCellWidth;
            const int x1 = left + (kGlyphWidth - runStart) * kCellWidth;
            addCuboid(*object, x0, y0, x1, y1);
        }
    }

    object->calculateBoundingBox();
    object->cullingMode = CullingMode::NO_CULLING;
    object->cachePositions();
    object->setPosition(0, 0, kDigitZ);
    return object;
}

bool allocateBuffers(std::uint16_t *&frameBuffer, std::uint16_t *&depthBuffer) {
    const std::size_t colorBytes =
        static_cast<std::size_t>(kPixelCount) * sizeof(std::uint16_t);
    const std::size_t depthCount =
        static_cast<std::size_t>(ZBUFFER_STRIDE(kScreenWidth)) * kScreenHeight;
    const std::size_t depthBytes = depthCount * sizeof(std::uint16_t);

    frameBuffer = static_cast<std::uint16_t *>(ps_malloc(colorBytes));
    if (frameBuffer == nullptr) {
        frameBuffer = static_cast<std::uint16_t *>(std::malloc(colorBytes));
    }
    depthBuffer = static_cast<std::uint16_t *>(ps_malloc(depthBytes));
    if (depthBuffer == nullptr) {
        depthBuffer = static_cast<std::uint16_t *>(std::malloc(depthBytes));
    }

    if (frameBuffer == nullptr || depthBuffer == nullptr) {
        std::free(frameBuffer);
        std::free(depthBuffer);
        frameBuffer = nullptr;
        depthBuffer = nullptr;
        return false;
    }
    return true;
}

void flushFrame(LilyGo_Glass &glass, std::uint16_t *frameBuffer) {
    glass.setAddrWindow(0, kDisplayOffsetY, kScreenWidth, kScreenHeight);
    glass.pushColors(frameBuffer, kPixelCount);
}

} // namespace

AstraGesture3DRenderer::AstraGesture3DRenderer(LilyGo_Glass &glass)
    : glass_(glass) {
}

AstraGesture3DRenderer::~AstraGesture3DRenderer() {
    end();
}

bool AstraGesture3DRenderer::begin() {
    if (available()) {
        return true;
    }
    if (!allocateBuffers(frameBuffer_, depthBuffer_)) {
        return false;
    }

    auto *scene = new Scene(frameBuffer_, depthBuffer_, kScreenWidth, kScreenHeight);
    auto *camera = new Camera();
    scene->setBackcolor(kBackground);
    scene->setClearBuffer(true);
    camera->setPosition(0, 0, 0);
    camera->setFOV(64.0F, kScreenWidth);
    camera->nearPlane = 64;
    camera->farPlane = 1400;
    scene->setCamera(camera);

    scene_ = scene;
    camera_ = camera;
    digit_ = astra_gesture::Digit::None;
    lastFrameMs_ = millis();
    animationSeconds_ = 0.0F;
    return true;
}

void AstraGesture3DRenderer::end() {
    auto *scene = static_cast<Scene *>(scene_);
    auto *object = static_cast<Object *>(object_);
    if (scene != nullptr && object != nullptr) {
        auto &objects = scene->getObjects();
        objects.erase(std::remove(objects.begin(), objects.end(), object), objects.end());
    }
    delete object;
    object_ = nullptr;
    delete static_cast<Scene *>(scene_);
    scene_ = nullptr;
    delete static_cast<Camera *>(camera_);
    camera_ = nullptr;
    std::free(frameBuffer_);
    std::free(depthBuffer_);
    frameBuffer_ = nullptr;
    depthBuffer_ = nullptr;
    digit_ = astra_gesture::Digit::None;
}

bool AstraGesture3DRenderer::available() const {
    return scene_ != nullptr && frameBuffer_ != nullptr && depthBuffer_ != nullptr;
}

const AstraGesture3DRenderer::DigitMask &AstraGesture3DRenderer::maskFor(
    astra_gesture::Digit digit) {
    static constexpr DigitMask kEmpty = {{0, 0, 0, 0, 0, 0, 0}};
    static constexpr DigitMask kMasks[] = {
        {{0, 0, 0, 0, 0, 0, 0}},
        {{0b00100, 0b01100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110}},
        {{0b01110, 0b10001, 0b00001, 0b00010, 0b00100, 0b01000, 0b11111}},
        {{0b01110, 0b10001, 0b00001, 0b00110, 0b00001, 0b10001, 0b01110}},
        {{0b00010, 0b00110, 0b01010, 0b10010, 0b11111, 0b00010, 0b00010}},
        {{0b11111, 0b10000, 0b11110, 0b00001, 0b00001, 0b10001, 0b01110}},
    };
    const auto index = static_cast<std::size_t>(digit);
    return index < (sizeof(kMasks) / sizeof(kMasks[0])) ? kMasks[index] : kEmpty;
}

bool AstraGesture3DRenderer::rebuildObject(astra_gesture::Digit digit) {
    if (!available()) {
        return false;
    }
    auto *scene = static_cast<Scene *>(scene_);
    auto *oldObject = static_cast<Object *>(object_);
    if (scene != nullptr && oldObject != nullptr) {
        auto &objects = scene->getObjects();
        objects.erase(std::remove(objects.begin(), objects.end(), oldObject), objects.end());
    }
    delete oldObject;
    object_ = nullptr;
    if (digit == astra_gesture::Digit::None) {
        digit_ = digit;
        return true;
    }

    auto *object = buildDigit(maskFor(digit));
    if (object == nullptr) {
        return false;
    }
    scene->addObject(object);
    object_ = object;
    digit_ = digit;
    return true;
}

void AstraGesture3DRenderer::setDigit(astra_gesture::Digit digit) {
    if (digit == digit_) {
        return;
    }
    rebuildObject(digit);
}

void AstraGesture3DRenderer::render() {
    if (!available()) {
        return;
    }

    const std::uint32_t now = millis();
    const std::uint32_t elapsedMs = now - lastFrameMs_;
    if (elapsedMs < kFrameIntervalMs) {
        return;
    }
    lastFrameMs_ = now;
    const std::uint32_t boundedMs = std::min<std::uint32_t>(elapsedMs, 100);
    animationSeconds_ += static_cast<float>(boundedMs) / 1000.0F;

    auto *scene = static_cast<Scene *>(scene_);
    auto *object = static_cast<Object *>(object_);
    if (object == nullptr) {
        std::fill(frameBuffer_, frameBuffer_ + kPixelCount, kBackground);
        flushFrame(glass_, frameBuffer_);
        return;
    }

    const int pitch = static_cast<int>(std::sin(animationSeconds_ * 0.9F) * 14.0F);
    const int yaw = static_cast<int>(std::sin(animationSeconds_ * 0.7F) * 28.0F);
    const int roll = static_cast<int>(std::sin(animationSeconds_ * 0.6F) * 6.0F);
    object->setRotation(pitch, yaw, roll);
    scene->render();
    flushFrame(glass_, frameBuffer_);
}
