#include <Arduino.h>
#include <LilyGo_GlassV3.h>
#include <pgmspace.h>

#include "imgData.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Standalone raw-RGB565 demo for the T-Glass V3 visible window.
// The game uses the bitmap and motion rules from the supplied dinosaur example.

namespace {

constexpr uint16_t kScreenWidth = 126;
constexpr uint16_t kScreenHeight = 126;
constexpr uint16_t kDisplayOffsetY = 168;
constexpr uint32_t kPixelCount =
    static_cast<uint32_t>(kScreenWidth) * static_cast<uint32_t>(kScreenHeight);

constexpr uint32_t kFrameIntervalMs = 16;
constexpr int16_t kGroundLine = 101;
constexpr int16_t kDinoGroundY = 75;
constexpr int16_t kJumpTopY = 0;
constexpr int16_t kObstacleY = 81;
constexpr int16_t kCloudY = 37;
constexpr int16_t kBumpY = 97;
constexpr int16_t kPlayerX = 30;

constexpr uint8_t kDinoWidth = 33;
constexpr uint8_t kDinoHeight = 35;
constexpr uint8_t kEnemyBitmapWidth = 18;
constexpr uint8_t kEnemyBitmapHeight = 38;
constexpr uint8_t kEnemyWidth = 14;
constexpr uint8_t kEnemyHeight = 30;
constexpr uint8_t kCloudWidth = 38;
constexpr uint8_t kCloudHeight = 11;
constexpr uint8_t kBumpWidth = 34;
constexpr uint8_t kBumpHeight = 5;

constexpr float kJumpStep = 1.4f;
constexpr float kInitialSpeed = 1.0f;
constexpr float kSpeedupGap = 0.1f;
constexpr float kCloudSpeed = 0.4f;
constexpr uint16_t kSpeedupScore = 100;
constexpr uint16_t kScoreIntervalMs = 120;

constexpr uint16_t kColorBackground = 0xFFFF;
constexpr uint16_t kColorInk = 0x0000;

const char kStartSound[] PROGMEM = "Start:d=32,o=5,b=240: c6, e6";
const char kJumpSound[] PROGMEM = "Jump:d=32,o=6,b=240: c7";
const char kScoreSound[] PROGMEM = "Score:d=32,o=6,b=240: e7, g7";
const char kGameOverSound[] PROGMEM = "GameOver:d=8,o=5,b=180: g4, c4, 2c4";

uint16_t *frameBuffer = nullptr;

enum class GameState : uint8_t {
    Menu,
    Running,
    GameOver,
};

struct Player {
    float y = kDinoGroundY;
    int8_t jumpDirection = 0;
    bool jumping = false;
};

struct Obstacle {
    float x = 0.0f;
    uint8_t bitmapFrame = 0;
};

struct Cloud {
    float x = 0.0f;
};

struct Bump {
    float x = 0.0f;
    uint8_t bitmapFrame = 0;
};

struct GroundMark {
    float x = 0.0f;
    uint8_t width = 1;
};

class EdgeButton {
public:
    EdgeButton(uint8_t pin, bool activeLow)
        : pin_(pin), activeLow_(activeLow)
    {
    }

    void begin()
    {
        pinMode(pin_, activeLow_ ? INPUT_PULLUP : INPUT);
        const bool active = readActive();
        rawState_ = active;
        stableState_ = active;
        changedAt_ = millis();
    }

    bool wasPressed()
    {
        const bool active = readActive();
        const uint32_t now = millis();

        if (active != rawState_) {
            rawState_ = active;
            changedAt_ = now;
        }

        if (active != stableState_ && now - changedAt_ >= 35) {
            stableState_ = active;
            return stableState_;
        }

        return false;
    }

private:
    bool readActive() const
    {
        const int level = digitalRead(pin_);
        return activeLow_ ? level == LOW : level == HIGH;
    }

    uint8_t pin_;
    bool activeLow_;
    bool rawState_ = false;
    bool stableState_ = false;
    uint32_t changedAt_ = 0;
};

EdgeButton startButton(BOARD_TOUCH_BUTTON, false);
EdgeButton jumpButton(BOARD_BOOT_PIN, true);

GameState gameState = GameState::Menu;
Player player;
Obstacle obstacles[2] = {{0.0f, 0}, {0.0f, 1}};
Cloud clouds[2];
Bump bumps[2];
GroundMark lines[6];
GroundMark lines2[6];

uint16_t score = 0;
uint16_t bestScore = 0;
uint16_t nextSpeedupScore = kSpeedupScore;
float obstacleSpeed = kInitialSpeed;
uint8_t frames = 0;
uint32_t runStartedAt = 0;
uint32_t lastFrameAt = 0;

// A compact 5x7 font. Each byte is one column, least significant bit is the top row.
const char kFontChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:.-/";
const uint8_t kFontData[][5] = {
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // A
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // F
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // G
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
    {0x41, 0x41, 0x7F, 0x41, 0x41}, // I
    {0x20, 0x40, 0x40, 0x40, 0x3F}, // J
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // M
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
    {0x46, 0x49, 0x49, 0x49, 0x31}, // S
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // T
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
    {0x7F, 0x20, 0x18, 0x20, 0x7F}, // W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // X
    {0x03, 0x04, 0x78, 0x04, 0x03}, // Y
    {0x61, 0x51, 0x49, 0x45, 0x43}, // Z
    {0x3E, 0x45, 0x49, 0x51, 0x3E}, // 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
    {0x62, 0x51, 0x49, 0x49, 0x46}, // 2
    {0x22, 0x41, 0x49, 0x49, 0x36}, // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
    {0x2F, 0x49, 0x49, 0x49, 0x31}, // 5
    {0x3E, 0x49, 0x49, 0x49, 0x30}, // 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 9
    {0x00, 0x36, 0x36, 0x00, 0x00}, // :
    {0x00, 0x60, 0x60, 0x00, 0x00}, // .
    {0x08, 0x08, 0x08, 0x08, 0x08}, // -
    {0x60, 0x10, 0x08, 0x04, 0x03}, // /
};

const uint8_t *glyphFor(char character)
{
    for (uint8_t index = 0; index < sizeof(kFontChars) - 1; ++index) {
        if (kFontChars[index] == character) {
            return kFontData[index];
        }
    }
    return nullptr;
}

void fillRect(int16_t x, int16_t y, int16_t width, int16_t height, uint16_t color)
{
    if (frameBuffer == nullptr || width <= 0 || height <= 0) {
        return;
    }

    int16_t xStart = x < 0 ? 0 : x;
    int16_t yStart = y < 0 ? 0 : y;
    int16_t xEnd = x + width;
    int16_t yEnd = y + height;
    if (xEnd > kScreenWidth) {
        xEnd = kScreenWidth;
    }
    if (yEnd > kScreenHeight) {
        yEnd = kScreenHeight;
    }
    if (xStart >= kScreenWidth || yStart >= kScreenHeight ||
        xStart >= xEnd || yStart >= yEnd) {
        return;
    }

    for (int16_t row = yStart; row < yEnd; ++row) {
        uint16_t *rowStart = frameBuffer + static_cast<uint32_t>(row) * kScreenWidth + xStart;
        for (int16_t column = xStart; column < xEnd; ++column) {
            *rowStart++ = color;
        }
    }
}

void drawPixel(int16_t x, int16_t y, uint16_t color)
{
    fillRect(x, y, 1, 1, color);
}

void drawRect(int16_t x, int16_t y, int16_t width, int16_t height, uint16_t color)
{
    if (width <= 0 || height <= 0) {
        return;
    }
    fillRect(x, y, width, 1, color);
    fillRect(x, y + height - 1, width, 1, color);
    fillRect(x, y, 1, height, color);
    fillRect(x + width - 1, y, 1, height, color);
}

int16_t textWidth(const char *text, uint8_t scale)
{
    if (text == nullptr || *text == '\0') {
        return 0;
    }

    int16_t width = 0;
    while (*text++ != '\0') {
        width += static_cast<int16_t>(6 * scale);
    }
    return width - scale;
}

void drawText(int16_t x, int16_t y, const char *text, uint16_t color, uint8_t scale)
{
    if (text == nullptr || scale == 0) {
        return;
    }

    while (*text != '\0') {
        const uint8_t *glyph = glyphFor(*text);
        if (glyph != nullptr) {
            for (uint8_t column = 0; column < 5; ++column) {
                const uint8_t columnBits = glyph[column];
                for (uint8_t row = 0; row < 7; ++row) {
                    if (columnBits & (1U << row)) {
                        fillRect(x + column * scale,
                                 y + row * scale,
                                 scale,
                                 scale,
                                 color);
                    }
                }
            }
        }
        x += 6 * scale;
        ++text;
    }
}

void drawCenteredText(int16_t y, const char *text, uint16_t color, uint8_t scale)
{
    const int16_t width = textWidth(text, scale);
    drawText((kScreenWidth - width) / 2, y, text, color, scale);
}

void drawXBitmap(int16_t x,
                int16_t y,
                const unsigned char *bitmap,
                uint8_t width,
                uint8_t height)
{
    if (bitmap == nullptr) {
        return;
    }

    const uint8_t bytesPerRow = static_cast<uint8_t>((width + 7) / 8);
    for (uint8_t row = 0; row < height; ++row) {
        for (uint8_t column = 0; column < width; ++column) {
            const uint8_t data = pgm_read_byte(bitmap + row * bytesPerRow + column / 8);
            if ((data & (1U << (column & 7))) != 0) {
                drawPixel(x + column, y + row, kColorInk);
            }
        }
    }
}

void drawXBitmapScaled(int16_t x,
                       int16_t y,
                       const unsigned char *bitmap,
                       uint8_t sourceWidth,
                       uint8_t sourceHeight,
                       uint8_t width,
                       uint8_t height)
{
    if (bitmap == nullptr || sourceWidth == 0 || sourceHeight == 0 ||
        width == 0 || height == 0) {
        return;
    }

    const uint8_t bytesPerRow = static_cast<uint8_t>((sourceWidth + 7) / 8);
    for (uint8_t row = 0; row < height; ++row) {
        const uint8_t sourceRow = static_cast<uint8_t>(
            (static_cast<uint16_t>(row) * sourceHeight) / height);
        for (uint8_t column = 0; column < width; ++column) {
            const uint8_t sourceColumn = static_cast<uint8_t>(
                (static_cast<uint16_t>(column) * sourceWidth) / width);
            const uint8_t data = pgm_read_byte(
                bitmap + sourceRow * bytesPerRow + sourceColumn / 8);
            if ((data & (1U << (sourceColumn & 7))) != 0) {
                drawPixel(x + column, y + row, kColorInk);
            }
        }
    }
}

void drawDino(int16_t x, int16_t y, uint8_t frame)
{
    drawXBitmap(x, y, dino[frame], kDinoWidth, kDinoHeight);
}

void drawClouds()
{
    for (uint8_t index = 0; index < 2; ++index) {
        drawXBitmap(static_cast<int16_t>(clouds[index].x + 0.5f),
                    kCloudY,
                    cloud,
                    kCloudWidth,
                    kCloudHeight);
    }
}

void drawGround()
{
    fillRect(0, kGroundLine, kScreenWidth, 1, kColorInk);
    for (uint8_t index = 0; index < 6; ++index) {
        fillRect(static_cast<int16_t>(lines[index].x + 0.5f),
                 kGroundLine + 16,
                 lines[index].width,
                 1,
                 kColorInk);
        fillRect(static_cast<int16_t>(lines2[index].x + 0.5f),
                 kGroundLine + 14,
                 lines2[index].width,
                 1,
                 kColorInk);
    }

    for (uint8_t index = 0; index < 2; ++index) {
        drawXBitmap(static_cast<int16_t>(bumps[index].x + 0.5f),
                    kBumpY,
                    bump[bumps[index].bitmapFrame],
                    kBumpWidth,
                    kBumpHeight);
    }
}

void drawObstacles()
{
    for (uint8_t index = 0; index < 2; ++index) {
        drawXBitmapScaled(static_cast<int16_t>(obstacles[index].x + 0.5f),
                          kObstacleY,
                          enemy[index],
                          kEnemyBitmapWidth,
                          kEnemyBitmapHeight,
                          kEnemyWidth,
                          kEnemyHeight);
    }
}

bool hasCollision(const Obstacle &obstacle)
{
    // Insets follow the transparent margins in the supplied sprites.
    constexpr int16_t kDinoInsetX = 4;
    constexpr int16_t kDinoInsetY = 4;
    constexpr int16_t kDinoHitWidth = 25;
    constexpr int16_t kDinoHitHeight = 27;
    constexpr int16_t kEnemyInsetX = 1;
    constexpr int16_t kEnemyInsetY = 1;
    constexpr int16_t kEnemyHitWidth = 12;
    constexpr int16_t kEnemyHitHeight = 28;

    const int16_t dinoLeft = kPlayerX + kDinoInsetX;
    const int16_t dinoTop = static_cast<int16_t>(player.y + 0.5f) + kDinoInsetY;
    const int16_t enemyLeft = static_cast<int16_t>(obstacle.x + 0.5f) + kEnemyInsetX;
    const int16_t enemyTop = kObstacleY + kEnemyInsetY;

    return dinoLeft < enemyLeft + kEnemyHitWidth &&
           dinoLeft + kDinoHitWidth > enemyLeft &&
           dinoTop < enemyTop + kEnemyHitHeight &&
           dinoTop + kDinoHitHeight > enemyTop;
}

void flushFrame()
{
    if (frameBuffer == nullptr) {
        return;
    }

    glass.setAddrWindow(0, kDisplayOffsetY, kScreenWidth, kScreenHeight);
    glass.pushColors(frameBuffer, kPixelCount);
}

void playSound(const char *rtttl)
{
    if (rtttl == nullptr || glass.audioOut == nullptr ||
        glass.rtttlFile == nullptr || glass.i2sRtttl == nullptr) {
        return;
    }

    if (glass.i2sRtttl->isRunning()) {
        glass.i2sRtttl->stop();
    }

    if (glass.rtttlFile->open(rtttl, strlen(rtttl))) {
        glass.i2sRtttl->begin(glass.rtttlFile, glass.audioOut);
    }
}

void serviceSound()
{
    if (glass.i2sRtttl == nullptr || !glass.i2sRtttl->isRunning()) {
        return;
    }

    if (!glass.i2sRtttl->loop()) {
        glass.i2sRtttl->stop();
    }
}

void resetSceneElements()
{
    clouds[0].x = static_cast<float>(random(0, 80));
    clouds[1].x = static_cast<float>(random(100, 180));

    for (uint8_t index = 0; index < 2; ++index) {
        bumps[index].x = static_cast<float>(random(index * 60, (index + 1) * 80));
        bumps[index].bitmapFrame = static_cast<uint8_t>(random(0, 2));
    }

    for (uint8_t index = 0; index < 6; ++index) {
        lines[index].x = static_cast<float>(random(index * 20, (index + 1) * 20));
        lines[index].width = static_cast<uint8_t>(random(1, 14));
        lines2[index].x = static_cast<float>(random(index * 20, (index + 1) * 20));
        lines2[index].width = static_cast<uint8_t>(random(1, 14));
    }
}

float respawnObstacleX(uint8_t index)
{
    const uint8_t otherIndex = index == 0 ? 1 : 0;
    const float minimum = static_cast<float>(kScreenWidth + 50);
    const float afterOther = obstacles[otherIndex].x + kEnemyWidth + 72.0f;
    const float start = afterOther > minimum ? afterOther : minimum;
    return start + static_cast<float>(random(0, 31));
}

void resetGame()
{
    player.y = kDinoGroundY;
    player.jumpDirection = 0;
    player.jumping = false;

    obstacles[0].x = static_cast<float>(random(kScreenWidth + 50, kScreenWidth + 81));
    obstacles[1].x = obstacles[0].x + static_cast<float>(random(92, 126));
    obstacles[0].bitmapFrame = 0;
    obstacles[1].bitmapFrame = 1;

    score = 0;
    nextSpeedupScore = kSpeedupScore;
    obstacleSpeed = kInitialSpeed;
    frames = 0;
    resetSceneElements();
}

void startGame()
{
    resetGame();
    gameState = GameState::Running;
    runStartedAt = millis();
    lastFrameAt = runStartedAt;
    playSound(kStartSound);
}

void finishGame()
{
    gameState = GameState::GameOver;
    if (score > bestScore) {
        bestScore = score;
    }
    playSound(kGameOverSound);
}

void handleStartAction()
{
    if (gameState == GameState::Menu || gameState == GameState::GameOver) {
        startGame();
    }
}

void handleJumpAction()
{
    if (gameState != GameState::Running || player.jumping) {
        return;
    }

    player.jumping = true;
    player.jumpDirection = -1;
    playSound(kJumpSound);
}

void updateClouds()
{
    for (uint8_t index = 0; index < 2; ++index) {
        clouds[index].x -= kCloudSpeed;
        if (clouds[index].x < -40.0f) {
            clouds[index].x = static_cast<float>(random(kScreenWidth, kScreenWidth + 45));
        }
    }
}

void updateGroundMarks()
{
    for (uint8_t index = 0; index < 6; ++index) {
        lines[index].x -= obstacleSpeed;
        if (lines[index].x < -14.0f) {
            lines[index].x = static_cast<float>(random(kScreenWidth + 119, kScreenWidth + 195));
            lines[index].width = static_cast<uint8_t>(random(1, 14));
        }

        lines2[index].x -= obstacleSpeed;
        if (lines2[index].x < -14.0f) {
            lines2[index].x = static_cast<float>(random(kScreenWidth + 119, kScreenWidth + 195));
            lines2[index].width = static_cast<uint8_t>(random(1, 14));
        }
    }
}

void updateBumps()
{
    for (uint8_t index = 0; index < 2; ++index) {
        bumps[index].x -= obstacleSpeed;
        if (bumps[index].x < -40.0f) {
            bumps[index].x = static_cast<float>(random(kScreenWidth, kScreenWidth + 45));
            bumps[index].bitmapFrame = static_cast<uint8_t>(random(0, 2));
        }
    }
}

void updateGame()
{
    if (gameState != GameState::Running) {
        return;
    }

    if (player.jumping) {
        player.y += static_cast<float>(player.jumpDirection) * kJumpStep;
        if (player.y <= kJumpTopY) {
            player.y = kJumpTopY;
            player.jumpDirection = 1;
        } else if (player.y >= kDinoGroundY) {
            player.y = kDinoGroundY;
            player.jumpDirection = 0;
            player.jumping = false;
        }
    }

    for (uint8_t index = 0; index < 2; ++index) {
        obstacles[index].x -= obstacleSpeed;
        if (obstacles[index].x < -30.0f) {
            obstacles[index].x = respawnObstacleX(index);
        }
    }

    score = static_cast<uint16_t>((millis() - runStartedAt) / kScoreIntervalMs);
    while (score >= nextSpeedupScore) {
        obstacleSpeed += kSpeedupGap;
        nextSpeedupScore += kSpeedupScore;
        playSound(kScoreSound);
    }

    for (uint8_t index = 0; index < 2; ++index) {
        if (hasCollision(obstacles[index])) {
            finishGame();
            break;
        }
    }

    ++frames;
    if (frames == 16) {
        frames = 0;
    }
}

void updateScene()
{
    updateClouds();
    if (gameState == GameState::Running) {
        updateGroundMarks();
        updateBumps();
    }
}

void drawBackground()
{
    fillRect(0, 0, kScreenWidth, kScreenHeight, kColorBackground);
    drawClouds();
}

void drawScore()
{
    char scoreText[6] = {0};
    snprintf(scoreText, sizeof(scoreText), "%04u", static_cast<unsigned>(score));
    drawText(96, 6, scoreText, kColorInk, 1);
}

void drawMenu()
{
    drawCenteredText(7, "DINO RUN", kColorInk, 2);
    drawDino(46, 43, 1);
    drawCenteredText(86, "TOUCH START", kColorInk, 1);
    drawCenteredText(96, "BOOT JUMP", kColorInk, 1);
}

void drawGameOver()
{
    fillRect(10, 33, kScreenWidth - 20, 45, kColorBackground);
    drawRect(10, 33, kScreenWidth - 20, 45, kColorInk);
    drawCenteredText(40, "GAME OVER", kColorInk, 2);

    char scoreText[12] = {0};
    snprintf(scoreText, sizeof(scoreText), "SCORE:%04u", static_cast<unsigned>(score));
    drawCenteredText(59, scoreText, kColorInk, 1);

    char bestText[12] = {0};
    snprintf(bestText, sizeof(bestText), "BEST:%04u", static_cast<unsigned>(bestScore));
    drawCenteredText(68, bestText, kColorInk, 1);
    drawCenteredText(88, "TOUCH RETRY", kColorInk, 1);
}

void renderFrame()
{
    drawBackground();
    drawGround();

    if (gameState == GameState::Menu) {
        drawMenu();
    } else {
        drawObstacles();
        const uint8_t frame = player.jumping ? 0 : (frames < 9 ? 1 : 2);
        drawDino(kPlayerX, static_cast<int16_t>(player.y + 0.5f), frame);
        drawScore();
        if (gameState == GameState::GameOver) {
            drawGameOver();
        }
    }

    flushFrame();
}

bool allocateFrameBuffer()
{
    frameBuffer = static_cast<uint16_t *>(ps_malloc(kPixelCount * sizeof(uint16_t)));
    if (frameBuffer == nullptr) {
        frameBuffer = static_cast<uint16_t *>(malloc(kPixelCount * sizeof(uint16_t)));
    }
    return frameBuffer != nullptr;
}

} // namespace

void setup()
{
    Serial.begin(115200);
    randomSeed(micros());

    if (!glass.begin()) {
        while (true) {
            Serial.println("T-Glass V3 initialization failed");
            delay(1000);
        }
    }

    glass.setBrightness(255);

    if (!allocateFrameBuffer()) {
        while (true) {
            Serial.println("DinoJump framebuffer allocation failed");
            delay(1000);
        }
    }

    startButton.begin();
    jumpButton.begin();
    resetGame();
    lastFrameAt = millis();
    renderFrame();
}

void loop()
{
    serviceSound();

    if (startButton.wasPressed()) {
        handleStartAction();
    }
    if (jumpButton.wasPressed()) {
        handleJumpAction();
    }

    const uint32_t now = millis();
    if (now - lastFrameAt >= kFrameIntervalMs) {
        lastFrameAt = now;
        updateScene();
        updateGame();
        renderFrame();
    }

    delay(1);
}
