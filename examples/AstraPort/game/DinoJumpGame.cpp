#include "DinoJumpGame.h"

#include <Arduino.h>

#include <algorithm>
#include <cmath>

namespace {

constexpr std::uint32_t kFrameIntervalMs = 16;
constexpr float kJumpStep = 1.4f;
constexpr float kInitialSpeed = 1.0f;
constexpr float kSpeedupGap = 0.1f;
constexpr float kMaximumSpeed = 2.0f;
constexpr float kCloudSpeed = 0.35f;
constexpr std::uint16_t kSpeedupScore = 100;
constexpr std::uint16_t kScoreIntervalMs = 120;
constexpr std::int16_t kPlayerX = 16;
constexpr std::uint8_t kCloudSourceWidth = 38;
constexpr std::uint8_t kBumpSourceWidth = 34;
constexpr std::int16_t kDinoInsetX = 2;
constexpr std::int16_t kDinoInsetY = 3;
constexpr std::int16_t kDinoHitWidth = 16;
constexpr std::int16_t kDinoHitHeight = 17;
constexpr std::int16_t kEnemyInsetX = 1;
constexpr std::int16_t kEnemyInsetY = 1;
constexpr std::int16_t kEnemyHitWidth = 4;
constexpr std::int16_t kEnemyHitHeight = 11;

} // namespace

DinoJumpGame::DinoJumpGame() {
    reset(0);
}

void DinoJumpGame::reset(std::uint32_t now) {
    resetGame();
    state_ = DinoJumpState::Menu;
    pendingEvent_ = DinoJumpEvent::None;
    lastFrameAt_ = now;
}

void DinoJumpGame::startOrJump(std::uint32_t now) {
    if (state_ == DinoJumpState::Menu || state_ == DinoJumpState::GameOver) {
        resetGame();
        state_ = DinoJumpState::Running;
        runStartedAt_ = now;
        lastFrameAt_ = now;
        emitEvent(DinoJumpEvent::Start);
        return;
    }

    jump(now);
}

void DinoJumpGame::jump(std::uint32_t now) {
    (void)now;
    if (state_ != DinoJumpState::Running || player_.jumping) return;

    player_.jumping = true;
    player_.jumpDirection = -1;
    emitEvent(DinoJumpEvent::Jump);
}

void DinoJumpGame::update(std::uint32_t now) {
    if (state_ != DinoJumpState::Running || now - lastFrameAt_ < kFrameIntervalMs) {
        return;
    }

    lastFrameAt_ = now;
    updateStep();
}

DinoJumpState DinoJumpGame::state() const {
    return state_;
}

bool DinoJumpGame::isGameOver() const {
    return state_ == DinoJumpState::GameOver;
}

DinoJumpView DinoJumpGame::view() const {
    DinoJumpView result;
    result.state = state_;
    result.playerY = player_.y;
    result.jumping = player_.jumping;
    result.animationFrame = player_.jumping ? 0 : (animationFrame_ < 8 ? 1 : 2);
    result.score = score_;
    result.bestScore = bestScore_;

    for (std::uint8_t index = 0; index < 2; ++index) {
        result.obstacleX[index] = obstacles_[index].x;
        result.obstacleFrame[index] = obstacles_[index].bitmapFrame;
        result.cloudX[index] = clouds_[index].x;
        result.bumpX[index] = bumps_[index].x;
        result.bumpFrame[index] = bumps_[index].bitmapFrame;
    }

    for (std::uint8_t index = 0; index < 6; ++index) {
        result.groundMarkX[index] = groundMarks_[index].x;
        result.groundMarkWidth[index] = groundMarks_[index].width;
        result.groundMark2X[index] = groundMarks2_[index].x;
        result.groundMark2Width[index] = groundMarks2_[index].width;
    }
    return result;
}

DinoJumpEvent DinoJumpGame::consumeEvent() {
    const DinoJumpEvent event = pendingEvent_;
    pendingEvent_ = DinoJumpEvent::None;
    return event;
}

void DinoJumpGame::resetGame() {
    player_.y = static_cast<float>(kDinoGroundY);
    player_.jumpDirection = 0;
    player_.jumping = false;

    obstacles_[0].x = static_cast<float>(random(kLogicalWidth + 30, kLogicalWidth + 56));
    obstacles_[1].x = obstacles_[0].x + static_cast<float>(random(70, 101));
    obstacles_[0].bitmapFrame = 0;
    obstacles_[1].bitmapFrame = 1;

    score_ = 0;
    nextSpeedupScore_ = kSpeedupScore;
    obstacleSpeed_ = kInitialSpeed;
    animationFrame_ = 0;
    resetSceneElements();
}

void DinoJumpGame::resetSceneElements() {
    clouds_[0].x = static_cast<float>(random(0, 65));
    clouds_[1].x = static_cast<float>(random(78, 145));

    for (std::uint8_t index = 0; index < 2; ++index) {
        bumps_[index].x = static_cast<float>(random(index * 48, (index + 1) * 64));
        bumps_[index].bitmapFrame = static_cast<std::uint8_t>(random(0, 2));
    }

    for (std::uint8_t index = 0; index < 6; ++index) {
        groundMarks_[index].x = static_cast<float>(random(index * 21, (index + 1) * 21));
        groundMarks_[index].width = static_cast<std::uint8_t>(random(1, 9));
        groundMarks2_[index].x = static_cast<float>(random(index * 21, (index + 1) * 21));
        groundMarks2_[index].width = static_cast<std::uint8_t>(random(1, 9));
    }
}

void DinoJumpGame::updateStep() {
    updateClouds();
    updateGroundMarks();
    updateBumps();
    updatePlayer();
    updateObstacles();

    score_ = static_cast<std::uint16_t>((millis() - runStartedAt_) / kScoreIntervalMs);
    while (score_ >= nextSpeedupScore_) {
        obstacleSpeed_ = std::min(obstacleSpeed_ + kSpeedupGap, kMaximumSpeed);
        nextSpeedupScore_ += kSpeedupScore;
        emitEvent(DinoJumpEvent::Score);
    }

    for (std::uint8_t index = 0; index < 2; ++index) {
        if (hasCollision(obstacles_[index])) {
            finishGame();
            break;
        }
    }

    animationFrame_ = static_cast<std::uint8_t>((animationFrame_ + 1) % 16);
}

void DinoJumpGame::updatePlayer() {
    if (!player_.jumping) return;

    player_.y += static_cast<float>(player_.jumpDirection) * kJumpStep;
    if (player_.y <= kJumpTopY) {
        player_.y = static_cast<float>(kJumpTopY);
        player_.jumpDirection = 1;
    } else if (player_.y >= kDinoGroundY) {
        player_.y = static_cast<float>(kDinoGroundY);
        player_.jumpDirection = 0;
        player_.jumping = false;
    }
}

void DinoJumpGame::updateClouds() {
    for (std::uint8_t index = 0; index < 2; ++index) {
        clouds_[index].x -= kCloudSpeed;
        if (clouds_[index].x < -static_cast<float>(kCloudSourceWidth)) {
            clouds_[index].x = static_cast<float>(random(kLogicalWidth, kLogicalWidth + 42));
        }
    }
}

void DinoJumpGame::updateGroundMarks() {
    for (std::uint8_t index = 0; index < 6; ++index) {
        groundMarks_[index].x -= obstacleSpeed_;
        if (groundMarks_[index].x < -14.0f) {
            groundMarks_[index].x = static_cast<float>(random(kLogicalWidth + 18,
                                                               kLogicalWidth + 68));
            groundMarks_[index].width = static_cast<std::uint8_t>(random(1, 9));
        }

        groundMarks2_[index].x -= obstacleSpeed_;
        if (groundMarks2_[index].x < -14.0f) {
            groundMarks2_[index].x = static_cast<float>(random(kLogicalWidth + 18,
                                                                kLogicalWidth + 68));
            groundMarks2_[index].width = static_cast<std::uint8_t>(random(1, 9));
        }
    }
}

void DinoJumpGame::updateBumps() {
    for (std::uint8_t index = 0; index < 2; ++index) {
        bumps_[index].x -= obstacleSpeed_;
        if (bumps_[index].x < -static_cast<float>(kBumpSourceWidth)) {
            bumps_[index].x = static_cast<float>(random(kLogicalWidth, kLogicalWidth + 42));
            bumps_[index].bitmapFrame = static_cast<std::uint8_t>(random(0, 2));
        }
    }
}

void DinoJumpGame::updateObstacles() {
    for (std::uint8_t index = 0; index < 2; ++index) {
        obstacles_[index].x -= obstacleSpeed_;
        if (obstacles_[index].x < -static_cast<float>(kEnemyRenderWidth + 12)) {
            obstacles_[index].x = respawnObstacleX(index);
        }
    }
}

void DinoJumpGame::finishGame() {
    state_ = DinoJumpState::GameOver;
    if (score_ > bestScore_) bestScore_ = score_;
    emitEvent(DinoJumpEvent::GameOver);
}

bool DinoJumpGame::hasCollision(const Obstacle &obstacle) const {
    const std::int16_t dinoLeft = kPlayerX + kDinoInsetX;
    const std::int16_t dinoTop = static_cast<std::int16_t>(player_.y + 0.5f) + kDinoInsetY;
    const std::int16_t enemyLeft = static_cast<std::int16_t>(obstacle.x + 0.5f) + kEnemyInsetX;
    const std::int16_t enemyTop = kObstacleY + kEnemyInsetY;

    return dinoLeft < enemyLeft + kEnemyHitWidth &&
           dinoLeft + kDinoHitWidth > enemyLeft &&
           dinoTop < enemyTop + kEnemyHitHeight &&
           dinoTop + kDinoHitHeight > enemyTop;
}

float DinoJumpGame::respawnObstacleX(std::uint8_t index) const {
    const std::uint8_t otherIndex = index == 0 ? 1 : 0;
    const float minimum = static_cast<float>(kLogicalWidth + 35);
    const float afterOther = obstacles_[otherIndex].x + kEnemyRenderWidth +
                             static_cast<float>(kMinimumObstacleGap);
    const float start = std::max(minimum, afterOther);
    return start + static_cast<float>(random(0, 21));
}

void DinoJumpGame::emitEvent(DinoJumpEvent event) {
    if (event == DinoJumpEvent::GameOver || pendingEvent_ == DinoJumpEvent::None) {
        pendingEvent_ = event;
    }
}
