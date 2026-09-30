#pragma once

#include <cstdint>

enum class DinoJumpState : std::uint8_t {
    Menu,
    Running,
    GameOver,
};

enum class DinoJumpEvent : std::uint8_t {
    None,
    Start,
    Jump,
    Score,
    GameOver,
};

struct DinoJumpView {
    DinoJumpState state = DinoJumpState::Menu;
    float playerY = 0.0f;
    bool jumping = false;
    std::uint8_t animationFrame = 1;
    std::uint16_t score = 0;
    std::uint16_t bestScore = 0;
    float obstacleX[2] = {};
    std::uint8_t obstacleFrame[2] = {};
    float cloudX[2] = {};
    float bumpX[2] = {};
    std::uint8_t bumpFrame[2] = {};
    float groundMarkX[6] = {};
    std::uint8_t groundMarkWidth[6] = {};
    float groundMark2X[6] = {};
    std::uint8_t groundMark2Width[6] = {};
};

class DinoJumpGame {
public:
    static constexpr std::int16_t kLogicalWidth = 128;
    static constexpr std::int16_t kLogicalHeight = 64;
    static constexpr std::int16_t kGroundLine = 51;
    static constexpr std::int16_t kDinoGroundY = 29;
    static constexpr std::int16_t kJumpTopY = 0;
    static constexpr std::int16_t kObstacleY = 38;
    static constexpr std::uint8_t kDinoRenderWidth = 20;
    static constexpr std::uint8_t kDinoRenderHeight = 21;
    static constexpr std::uint8_t kEnemyRenderWidth = 6;
    static constexpr std::uint8_t kEnemyRenderHeight = 13;
    static constexpr std::int16_t kMinimumObstacleGap = 50;

    DinoJumpGame();

    void reset(std::uint32_t now);
    void startOrJump(std::uint32_t now);
    void jump(std::uint32_t now);
    void update(std::uint32_t now);

    DinoJumpState state() const;
    bool isGameOver() const;
    DinoJumpView view() const;
    DinoJumpEvent consumeEvent();

private:
    struct Player {
        float y = static_cast<float>(kDinoGroundY);
        std::int8_t jumpDirection = 0;
        bool jumping = false;
    };

    struct Obstacle {
        float x = 0.0f;
        std::uint8_t bitmapFrame = 0;
    };

    struct Cloud {
        float x = 0.0f;
    };

    struct Bump {
        float x = 0.0f;
        std::uint8_t bitmapFrame = 0;
    };

    struct GroundMark {
        float x = 0.0f;
        std::uint8_t width = 1;
    };

    void resetGame();
    void resetSceneElements();
    void updateStep();
    void updatePlayer();
    void updateClouds();
    void updateGroundMarks();
    void updateBumps();
    void updateObstacles();
    void finishGame();
    bool hasCollision(const Obstacle &obstacle) const;
    float respawnObstacleX(std::uint8_t index) const;
    void emitEvent(DinoJumpEvent event);

    DinoJumpState state_ = DinoJumpState::Menu;
    Player player_;
    Obstacle obstacles_[2];
    Cloud clouds_[2];
    Bump bumps_[2];
    GroundMark groundMarks_[6];
    GroundMark groundMarks2_[6];

    std::uint16_t score_ = 0;
    std::uint16_t bestScore_ = 0;
    std::uint16_t nextSpeedupScore_ = 100;
    float obstacleSpeed_ = 1.0f;
    std::uint8_t animationFrame_ = 0;
    std::uint32_t runStartedAt_ = 0;
    std::uint32_t lastFrameAt_ = 0;
    DinoJumpEvent pendingEvent_ = DinoJumpEvent::None;
};
