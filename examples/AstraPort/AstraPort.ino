#include <Arduino.h>

#include <AudioLogger.h>

#include <cstdint>

#include <LilyGo_GlassV3.h>

#include "AstraDinoJumpController.h"
#include "AstraGlassServices.h"
#include "AstraPortModel.h"
#include "Astra3DIntro.h"
#include "astra/ui/launcher.h"
#include "hal/AstraGlassHal.h"

namespace {

astra::Launcher *astraLauncher = nullptr;
AstraGlassServices *astraServices = nullptr;
AstraDinoJumpController *astraDinoJump = nullptr;

void stopOnFailure() {
    while (true) {
        Serial.println("Factory_Astra: glass.begin() failed");
        delay(1000);
    }
}

} // namespace

void setup() {
    Serial.begin(115200);
    audioLogger = &Serial;

    if (!glass.begin()) {
        stopOnFailure();
    }
    glass.setBrightness(255);
    astra::run3DIntro(glass);

    auto *astraHal = new AstraGlassHal(glass);
    if (!HAL::inject(astraHal)) {
        stopOnFailure();
    }

    astraServices = new AstraGlassServices(glass, *astraHal);
    astraServices->begin();

    astraDinoJump = new AstraDinoJumpController(*astraHal, *astraServices);
    const AstraPortPages pages = buildAstraPortPages(*astraServices, *astraDinoJump);
    astraLauncher = new astra::Launcher();
    astraLauncher->init(pages.root);

}

void loop() {
    if (astraServices != nullptr) {
        astraServices->update();
    }
    if (astraServices != nullptr && astraLauncher != nullptr) {
        astra_gesture_control::Action action = astra_gesture_control::Action::None;
        while (astraServices->takeGestureAction(action)) {
            if (action == astra_gesture_control::Action::TogglePlayback) {
                astraServices->toggleRadioPlayback();
            } else {
                astraLauncher->applyGestureAction(action);
            }
        }
    }
    if (astraDinoJump != nullptr && astraDinoJump->isActive()) {
        astraDinoJump->update();
    } else if (astraLauncher != nullptr) {
        astraLauncher->update(false);
        if (astraServices != nullptr) {
            astraServices->renderStatusBar();
        }
        const bool gestureDisplayActive =
            astraServices != nullptr && astraServices->gesture3DDisplayActive();
        if (!gestureDisplayActive) {
            HAL::canvasUpdate();
        }
    }
    if (astraServices != nullptr) {
        astraServices->renderGesture3D();
    }
    glass.update();
    delay(5);
}
