#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include <LilyGo_GlassV3.h>
#include <RadioLib.h>
#include <WiFi.h>

#include "AstraGestureControl.h"
#include "AstraStatusBar.h"
#include "AstraPortSurface.h"
#include "AstraLanguage.h"
#include "AstraSettings.h"
#include "RadioDemoConfig.h"
#include "RadioDemoPlayer.h"
#include "camera/AstraCameraPipeline.h"
#include "camera/CameraWebServer/AstraCameraWebServer.h"
#include "camera/Gesture3D/AstraGesture3D.h"
#include "hal/AstraGlassHal.h"

#ifndef WIFI_SSID
#define WIFI_SSID "YOUR_WIFI_SSID"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#endif

#ifndef WIFI_SSID2
#define WIFI_SSID2 "YOUR_WIFI_SSID_2"
#endif

#ifndef WIFI_PASSWORD2
#define WIFI_PASSWORD2 "YOUR_WIFI_PASSWORD_2"
#endif

#ifndef ASTRA_WIFI_SSID
#define ASTRA_WIFI_SSID WIFI_SSID
#endif

#ifndef ASTRA_WIFI_PASSWORD
#define ASTRA_WIFI_PASSWORD WIFI_PASSWORD
#endif

#ifndef ASTRA_WIFI_SSID2
#define ASTRA_WIFI_SSID2 WIFI_SSID2
#endif

#ifndef ASTRA_WIFI_PASSWORD2
#define ASTRA_WIFI_PASSWORD2 WIFI_PASSWORD2
#endif

class AstraGlassServices {
public:
    AstraGlassServices(LilyGo_Glass &glass, AstraGlassHal &hal);
    ~AstraGlassServices();

    void begin();
    void update();

    void renderCameraStatus();
    void renderStatusBar() const;
    void renderGesture3DStatus();
    void renderGestureControlStatus();
    void renderCameraStreamStatus();
    void renderGesture3D();
    void renderMicrophone();
    void renderInputTest();
    void renderLoRaTransmit();
    void renderLoRaReceive();
    void renderLoRaParameters();
    void renderWirelessStatus();
    void renderWifiStatus();
    void renderWifiScan();
    void renderTime();
    void renderBattery();
    void renderDiagnostics();

    void startMicrophone();
    void stopMicrophone();
    void startLoRaTransmit();
    void stopLoRaTransmit();
    void startLoRaReceive();
    void stopLoRaReceive();
    void scanWifiNow();
    void playRtttl(const char *rtttl);
    void stopRtttl();
    void enterRadio();
    void exitRadio();
    const char *selectRadioStation(std::size_t index);
    void renderRadioDetail();
    void increaseRadioVolume();

    const char *speakerTest();
    const char *nudgeDisplay(std::int16_t deltaX, std::int16_t deltaY);
    const char *saveDisplayCalibration();
    const char *setBrightnessLevel(std::uint8_t brightness);
    const char *sleepNow();
    const char *shutdownNow();
    const char *takeShutdownNotice();

    bool &wifiEnabledSetting();
    bool &gestureEnabledSetting();
    bool &languageChineseSetting();
    bool &languageEnglishSetting();
    bool wifiEnabled() const;
    bool gestureEnabled() const;
    bool isEnglish() const;
    astra_settings::Language language() const;
    const char *setLanguage(astra_settings::Language language);
    const char *text(astra_language::TextId id) const;
    const char *toggleWifi();
    const char *toggleGesture();
    void enterGesture3D();
    void exitGesture3D();
    void enterGestureControl();
    void exitGestureControl();
    bool takeGestureAction(astra_gesture_control::Action &action);
    void toggleRadioPlayback();
    void enterCameraStream();
    void exitCameraStream();
    void enterScreenTest(AstraScreenPattern pattern);
    void advanceScreenTestPattern();
    void exitScreenTest();
    void renderScreenTest();
    bool screenTestActive() const;
    bool gesture3DDisplayActive() const;

private:
    struct LoRaSettings {
        float frequency = 868.0f;
        float bandwidth = 125.0f;
        std::uint8_t spreadingFactor = 10;
        std::uint8_t codingRate = 6;
        std::uint8_t syncWord = 0x12;
        std::uint8_t txPower = 22;
        std::uint16_t preambleLength = 15;
    };

    static void radioEventThunk();

    void loadPreferences();
    void savePreferences();
    bool configureRadio();
    void startLoRaTransmitPacket();
    void pollLoRaTransmit();
    void pollLoRaReceive();
    void updateAudioLevels();
    void updateRtttl();
    void updateBattery();
    void updateBootForceShutdown();
    void updateWifi();
    void updateGesture3D();
    bool startGesture3D();
    void printCameraStreamUrls() const;
    const char *currentWifiSsid() const;
    void startWifi();
    void stopWifi();

    void drawHeader(const char *title) const;
    void drawLine(const std::string &text, std::int16_t baseline) const;
    void drawCentered(const std::string &text, std::int16_t baseline) const;
    void drawBar(std::int16_t y, int value, int maximum) const;
    static std::string shorten(const std::string &text, std::size_t maximum);

    LilyGo_Glass &glass_;
    AstraGlassHal &hal_;
    AstraCameraPipeline cameraPipeline_;
    AstraCameraWebServer cameraWebServer_;
    AstraGesture3D gesture3D_;
    radio_demo::RadioDemoPlayer radioPlayer_;
    Module *radioModule_;
    SX1262 radio_;
    LoRaSettings loraSettings_;

    bool radioReady_ = false;
    bool transmitActive_ = false;
    bool transmitInFlight_ = false;
    bool receiveActive_ = false;
    volatile bool radioEvent_ = false;
    int radioState_ = 0;
    std::uint32_t nextTransmitAt_ = 0;
    std::uint32_t transmitCount_ = 0;
    std::string transmitPayload_;
    std::string lastReceived_ = "N.A";
    float lastRssi_ = 0.0f;
    float lastSnr_ = 0.0f;

    bool microphoneActive_ = false;
    bool microphoneReady_ = false;
    int audioLeft_ = 0;
    int audioRight_ = 0;

    std::uint16_t batteryVoltage_ = 0;
    int batteryPercent_ = 0;
    bool charging_ = false;
    std::uint32_t lastAudioUpdate_ = 0;
    std::uint32_t lastBatteryUpdate_ = 0;

    String scanNames_[4];
    int scanRssi_[4] = {};
    std::uint8_t scanCount_ = 0;

    std::int16_t displayOffsetX_ = 0;
    std::int16_t displayOffsetY_ = 0;
    std::uint8_t brightness_ = 255;

    bool wifiEnabled_ = astra_settings::defaultState().wifiEnabled;
    bool gestureEnabled_ = astra_settings::defaultState().gestureEnabled;
    astra_settings::Language language_ = astra_settings::defaultLanguage();
    bool languageChinese_ = true;
    bool languageEnglish_ = false;
    bool wifiConfigured_ = false;
    std::uint8_t wifiNetworkIndex_ = 0;
    bool gesturePageActive_ = false;
    bool gestureControlPageActive_ = false;
    bool gestureControlMode_ = false;
    astra_gesture_control::Action pendingGestureAction_ = astra_gesture_control::Action::None;
    bool cameraStreamPageActive_ = false;
    AstraScreenPattern screenPattern_ = AstraScreenPattern::White;
    bool screenTestActive_ = false;
    std::uint32_t lastWifiAttempt_ = 0;
    std::uint32_t bootPressedAt_ = 0;
    bool bootPressTracking_ = false;
    bool bootForceShutdownTriggered_ = false;
    const char *pendingShutdownNotice_ = nullptr;

    static AstraGlassServices *activeRadioService_;
};
