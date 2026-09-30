#include "AstraDinoJumpController.h"

#include <Arduino.h>

#include <algorithm>

#include "AstraGlassServices.h"
#include "hal/AstraGlassHal.h"

namespace {

constexpr char kStartSound[] = "Start:d=32,o=5,b=240:c6,e6";
constexpr char kJumpSound[] = "Jump:d=32,o=6,b=240:c7";
constexpr char kScoreSound[] = "Score:d=32,o=6,b=240:e7,g7";
constexpr char kGameOverSound[] = "GameOver:d=8,o=5,b=180:g4,c4,2c4";

} // namespace

AstraDinoJumpController::AstraDinoJumpController(AstraGlassHal &hal,
                                                 AstraGlassServices &services)
    : hal_(hal), services_(services) {
}

void AstraDinoJumpController::enter() {
    if (active_) return;

    randomSeed(HAL::getRandomSeed());
    game_.reset(millis());
    active_ = true;
}

void AstraDinoJumpController::leave() {
    if (!active_) return;

    active_ = false;
    services_.stopRtttl();
    game_.reset(millis());
    clearKeys();
}

void AstraDinoJumpController::update() {
    if (!active_) return;

    HAL::keyScan();
    const std::uint32_t now = millis();
    handleKeys(now);
    if (!active_) return;

    game_.update(now);
    playPendingSound();
    renderer_.render(hal_.surface(), game_.view());
    HAL::canvasUpdate();
}

bool AstraDinoJumpController::isActive() const {
    return active_;
}

void AstraDinoJumpController::handleKeys(std::uint32_t now) {
    if (*HAL::getKeyFlag() != key::KEY_PRESSED) return;

    key::KEY_ACTION *keyMap = HAL::getKeyMap();
    const bool exitRequested = keyMap[key::KEY_0] == key::PRESS;
    if (exitRequested) {
        clearKeys();
        leave();
        return;
    }

    if (keyMap[key::KEY_0] == key::CLICK) {
        game_.jump(now);
    }
    if (keyMap[key::KEY_1] == key::CLICK) {
        game_.startOrJump(now);
    }
    clearKeys();
}

void AstraDinoJumpController::clearKeys() {
    std::fill(HAL::getKeyMap(), HAL::getKeyMap() + key::KEY_NUM, key::INVALID);
    *HAL::getKeyFlag() = key::KEY_NOT_PRESSED;
}

void AstraDinoJumpController::playPendingSound() {
    switch (game_.consumeEvent()) {
        case DinoJumpEvent::Start:
            services_.playRtttl(kStartSound);
            break;
        case DinoJumpEvent::Jump:
            services_.playRtttl(kJumpSound);
            break;
        case DinoJumpEvent::Score:
            services_.playRtttl(kScoreSound);
            break;
        case DinoJumpEvent::GameOver:
            services_.playRtttl(kGameOverSound);
            break;
        case DinoJumpEvent::None:
        default:
            break;
    }
}
