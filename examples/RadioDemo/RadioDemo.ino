#include <Arduino.h>
#include <WiFi.h>

#include <AudioLogger.h>
#include <LilyGo_GlassV3.h>

#include <cstring>

#include "RadioDemoConfig.h"
#include "RadioDemoPlayer.h"

namespace {

class RadioDemoButton {
public:
    RadioDemoButton(int pin, bool activeHigh, unsigned long longPressMs = 1000)
        : pin_(pin),
          activeHigh_(activeHigh),
          longPressMs_(longPressMs) {
    }

    void begin() {
        pinMode(pin_, activeHigh_ ? INPUT : INPUT_PULLUP);
        lastPressed_ = false;
        pressedAt_ = 0;
        shortPress_ = false;
        longPress_ = false;
        longPressEmitted_ = false;
    }

    void update() {
        const int raw = digitalRead(pin_);
        const bool pressed = activeHigh_ ? raw == HIGH : raw == LOW;

        if (pressed && !lastPressed_) {
            pressedAt_ = millis();
            longPressEmitted_ = false;
        } else if (pressed && !longPressEmitted_ &&
                   millis() - pressedAt_ >= longPressMs_) {
            longPress_ = true;
            longPressEmitted_ = true;
        } else if (!pressed && lastPressed_ && !longPressEmitted_) {
            shortPress_ = true;
        }

        lastPressed_ = pressed;
    }

    bool consumeShortPress() {
        if (!shortPress_) return false;
        shortPress_ = false;
        return true;
    }

    bool consumeLongPress() {
        if (!longPress_) return false;
        longPress_ = false;
        return true;
    }

private:
    int pin_;
    bool activeHigh_;
    unsigned long longPressMs_;
    bool lastPressed_ = false;
    unsigned long pressedAt_ = 0;
    bool shortPress_ = false;
    bool longPress_ = false;
    bool longPressEmitted_ = false;
};

const char *stateLabel(radio_demo::RadioDemoState state) {
    switch (state) {
    case radio_demo::RadioDemoState::Idle:
        return "IDLE";
    case radio_demo::RadioDemoState::WaitingForWifi:
        return "WAIT_WIFI";
    case radio_demo::RadioDemoState::Connecting:
        return "CONNECTING";
    case radio_demo::RadioDemoState::Buffering:
        return "BUFFERING";
    case radio_demo::RadioDemoState::Playing:
        return "PLAYING";
    case radio_demo::RadioDemoState::Paused:
        return "PAUSED";
    case radio_demo::RadioDemoState::Retrying:
        return "RETRYING";
    case radio_demo::RadioDemoState::Error:
        return "ERROR";
    }
    return "UNKNOWN";
}

radio_demo::RadioDemoPlayer player;
RadioDemoButton bootButton(BOARD_BOOT_PIN, false);
RadioDemoButton touchButton(BOARD_TOUCH_BUTTON, true);
unsigned long lastWifiRetry = 0;

void logPlayerChanges() {
    static radio_demo::RadioDemoState lastState = radio_demo::RadioDemoState::Idle;
    static std::size_t lastStationIndex = static_cast<std::size_t>(-1);
    static std::uint8_t lastVolume = 0;
    static char lastTitle[128] = {};
    static bool hasLogged = false;

    const radio_demo::RadioDemoStation *station = player.station();
    const char *title = player.streamTitle();
    const bool changed = !hasLogged || player.state() != lastState ||
                         player.stationIndex() != lastStationIndex ||
                         player.volume() != lastVolume ||
                         std::strcmp(title, lastTitle) != 0;
    if (!changed) return;

    Serial.printf("RadioDemo: station=%s state=%s volume=%u title=%s\n",
                  station == nullptr ? "none" : station->name,
                  stateLabel(player.state()),
                  static_cast<unsigned>(player.volume()),
                  title[0] == '\0' ? "(no ICY title)" : title);

    lastState = player.state();
    lastStationIndex = player.stationIndex();
    lastVolume = player.volume();
    std::strncpy(lastTitle, title, sizeof(lastTitle) - 1);
    lastTitle[sizeof(lastTitle) - 1] = '\0';
    hasLogged = true;
}

}  // namespace

void setup() {
    Serial.begin(115200);
    audioLogger = &Serial;

    if (!glass.begin(false)) {
        while (true) {
            Serial.println("RadioDemo: glass.begin() failed");
            delay(1000);
        }
    }

    glass.setBrightness(255);

    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.setAutoReconnect(true);
    WiFi.begin(radio_demo::kWifiSsid, radio_demo::kWifiPassword);

    player.begin(glass.audioOut,
                 radio_demo::kStations,
                 radio_demo::kStationCount);
    bootButton.begin();
    touchButton.begin();

    Serial.printf("RadioDemo: connecting to %s\n", radio_demo::kWifiSsid);
    Serial.println("RadioDemo: TOUCH=next station, BOOT=play/pause, BOOT long=volume");
}

void loop() {
    touchButton.update();
    bootButton.update();

    if (touchButton.consumeShortPress()) {
        player.selectNextStation();
    }
    if (bootButton.consumeShortPress()) {
        player.togglePlayback();
    }
    if (bootButton.consumeLongPress()) {
        player.increaseVolume();
    }

    if (WiFi.status() != WL_CONNECTED && millis() - lastWifiRetry >= 5000) {
        lastWifiRetry = millis();
        WiFi.reconnect();
    }

    player.update();
    glass.update();
    logPlayerChanges();
    delay(2);
}
