#pragma once

#include <cstdint>

#include "game/DinoJumpGame.h"
#include "game/DinoJumpRenderer.h"

class AstraGlassHal;
class AstraGlassServices;

class AstraDinoJumpController {
public:
    AstraDinoJumpController(AstraGlassHal &hal, AstraGlassServices &services);

    void enter();
    void leave();
    void update();
    bool isActive() const;

private:
    void handleKeys(std::uint32_t now);
    void clearKeys();
    void playPendingSound();

    AstraGlassHal &hal_;
    AstraGlassServices &services_;
    DinoJumpGame game_;
    DinoJumpRenderer renderer_;
    bool active_ = false;
};
