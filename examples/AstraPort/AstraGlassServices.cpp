#include "AstraGlassServices.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <cstring>

#include <Arduino.h>
#include <Preferences.h>
#include <driver/rtc_io.h>
#include <esp_sntp.h>
#include <esp_sleep.h>

#include "fonts/astra_fonts.h"

namespace {

constexpr char kTimeZone[] = "CST-8";
constexpr char kNtpServerOne[] = "pool.ntp.org";
constexpr char kNtpServerTwo[] = "time.nist.gov";
constexpr std::int16_t kMinimumDisplayOffset = -63;
constexpr std::int16_t kMaximumDisplayOffset = 63;
constexpr std::uint16_t kRadioTextWidth = 124;
constexpr std::uint32_t kRadioMarqueeStepMs = 260;
constexpr std::uint32_t kLoRaTransmitIntervalMs = 1000;
constexpr std::uint32_t kWifiRetryIntervalMs = 10000;

struct WifiCredential {
    const char *ssid;
    const char *password;
};

constexpr WifiCredential kWifiCredentials[] = {
    {ASTRA_WIFI_SSID, ASTRA_WIFI_PASSWORD},
    {ASTRA_WIFI_SSID2, ASTRA_WIFI_PASSWORD2},
};
constexpr std::uint8_t kWifiCredentialCount =
    static_cast<std::uint8_t>(sizeof(kWifiCredentials) / sizeof(kWifiCredentials[0]));

std::string asStdString(const String &value) {
    return std::string(value.c_str());
}

std::size_t nextRadioUtf8CodePoint(const std::string &text, std::size_t offset) {
    if (offset >= text.size()) return text.size();

    const unsigned char first = static_cast<unsigned char>(text[offset]);
    std::size_t length = 1;
    if ((first & 0xE0U) == 0xC0U) {
        length = 2;
    } else if ((first & 0xF0U) == 0xE0U) {
        length = 3;
    } else if ((first & 0xF8U) == 0xF0U) {
        length = 4;
    }
    return std::min(offset + length, text.size());
}

std::size_t radioUtf8CodePointCount(const std::string &text) {
    std::size_t count = 0;
    for (std::size_t offset = 0; offset < text.size(); ++count) {
        offset = nextRadioUtf8CodePoint(text, offset);
    }
    return count;
}

std::size_t radioUtf8ByteOffset(const std::string &text, std::size_t codePoint) {
    std::size_t offset = 0;
    while (codePoint-- > 0 && offset < text.size()) {
        offset = nextRadioUtf8CodePoint(text, offset);
    }
    return offset;
}

std::uint16_t radioTextWidth(const std::string &text) {
    std::string measured = text;
    return HAL::getFontWidth(measured);
}

bool radioTextFits(const std::string &text) {
    std::string prefix;
    for (std::size_t offset = 0; offset < text.size();) {
        const std::size_t nextOffset = nextRadioUtf8CodePoint(text, offset);
        const std::string candidate = prefix + text.substr(offset, nextOffset - offset);
        if (radioTextWidth(candidate) > kRadioTextWidth) return false;
        prefix = candidate;
        offset = nextOffset;
    }
    return true;
}

std::string makeRadioMarquee(const std::string &text) {
    const std::size_t codePointCount = radioUtf8CodePointCount(text);
    if (codePointCount == 0 || radioTextFits(text)) return text;

    constexpr std::size_t kGapCodePoints = 4;
    const std::size_t cycleLength = codePointCount + kGapCodePoints;
    const std::size_t startCodePoint =
        (static_cast<std::uint32_t>(HAL::millis()) / kRadioMarqueeStepMs) % cycleLength;
    std::size_t sourceOffset = radioUtf8ByteOffset(
        text, startCodePoint < codePointCount ? startCodePoint : 0);
    std::size_t cyclePosition = startCodePoint;
    std::string result;

    for (std::size_t rendered = 0; rendered < cycleLength; ++rendered) {
        std::string unit;
        if (cyclePosition < codePointCount) {
            const std::size_t nextOffset = nextRadioUtf8CodePoint(text, sourceOffset);
            unit = text.substr(sourceOffset, nextOffset - sourceOffset);
            sourceOffset = nextOffset;
        } else {
            unit = " ";
        }

        const std::string candidate = result + unit;
        if (radioTextWidth(candidate) > kRadioTextWidth) break;
        result = candidate;
        cyclePosition = (cyclePosition + 1) % cycleLength;
        if (cyclePosition == 0) sourceOffset = 0;
    }

    if (!result.empty()) return result;
    return text.substr(0, nextRadioUtf8CodePoint(text, 0));
}

const char *radioStateLabel(radio_demo::RadioDemoState state,
                            astra_settings::Language language) {
    switch (state) {
        case radio_demo::RadioDemoState::WaitingForWifi:
            return astra_language::text(astra_language::TextId::RadioWaitingForWifi, language);
        case radio_demo::RadioDemoState::Connecting:
            return astra_language::text(astra_language::TextId::RadioConnecting, language);
        case radio_demo::RadioDemoState::Buffering:
            return astra_language::text(astra_language::TextId::RadioBuffering, language);
        case radio_demo::RadioDemoState::Playing:
            return astra_language::text(astra_language::TextId::RadioPlaying, language);
        case radio_demo::RadioDemoState::Paused:
            return astra_language::text(astra_language::TextId::RadioPaused, language);
        case radio_demo::RadioDemoState::Retrying:
            return astra_language::text(astra_language::TextId::RadioRetrying, language);
        case radio_demo::RadioDemoState::Error:
            return astra_language::text(astra_language::TextId::RadioError, language);
        case radio_demo::RadioDemoState::Idle:
        default:
            return astra_language::text(astra_language::TextId::RadioIdle, language);
    }
}

void drawRadioEqualizer(std::uint32_t animationPhase, bool animated) {
    constexpr std::int16_t kBaseY = 54;
    constexpr std::int16_t kStartX = 4;
    constexpr std::int16_t kBarWidth = 4;
    constexpr std::int16_t kBarGap = 3;
    constexpr std::uint8_t kPattern[5] = {0, 2, 4, 1, 3};

    for (std::size_t index = 0; index < 5; ++index) {
        const std::int16_t height = animated
                                        ? static_cast<std::int16_t>(4 +
                                            ((animationPhase + kPattern[index]) % 5) * 2)
                                        : 3;
        HAL::drawBox(kStartX + static_cast<std::int16_t>(index) * (kBarWidth + kBarGap),
                     kBaseY - height,
                     kBarWidth,
                     height);
    }
}

void drawRadioText(const std::string &text, std::int16_t x, std::int16_t baseline) {
    HAL::setFont(u8g2_font_wqy12_t_gb2312);
    HAL::setDrawType(1);
    HAL::drawChinese(x, baseline, text);
}

} // namespace

AstraGlassServices *AstraGlassServices::activeRadioService_ = nullptr;

AstraGlassServices::AstraGlassServices(LilyGo_Glass &glass, AstraGlassHal &hal)
    : glass_(glass),
      hal_(hal),
      cameraPipeline_(glass.isCameraDetected()),
      cameraWebServer_(cameraPipeline_),
      gesture3D_(cameraPipeline_, glass),
      radioModule_(new Module(LORA_CS, LORA_IRQ, LORA_RST, LORA_BUSY)),
      radio_(radioModule_) {
}

AstraGlassServices::~AstraGlassServices() {
    exitGesture3D();
    stopLoRaTransmit();
    cameraWebServer_.stop();
    delete radioModule_;
    radioModule_ = nullptr;
}

void AstraGlassServices::begin() {
    loadPreferences();

    activeRadioService_ = this;
    radio_.setDio1Action(radioEventThunk);
    radioState_ = radio_.begin();
    radioReady_ = radioState_ == RADIOLIB_ERR_NONE;
    if (radioReady_) {
        if (configureRadio()) {
            radio_.sleep(true);
        } else {
            radioReady_ = false;
        }
    }

    if (wifiEnabled_) {
        startWifi();
    } else {
        stopWifi();
    }

    radioPlayer_.begin(glass_.audioOut,
                       radio_demo::kStations,
                       radio_demo::kStationCount);
    radioPlayer_.setPlaybackEnabled(false);
}

void AstraGlassServices::update() {
    updateWifi();
    updateGesture3D();
    radioPlayer_.update();
    updateBattery();
    updateAudioLevels();
    updateRtttl();
    if (transmitActive_) {
        pollLoRaTransmit();
    } else if (receiveActive_) {
        pollLoRaReceive();
    }
}

void AstraGlassServices::startWifi() {
    if (!wifiEnabled_) {
        return;
    }

    const WifiCredential &credential =
        kWifiCredentials[wifiNetworkIndex_ % kWifiCredentialCount];
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    // Let Factory_Astra control retries so a reconnect cannot race Arduino's
    // internal reconnect handler while ESP-DL is using PSRAM.
    WiFi.setAutoReconnect(false);
    WiFi.disconnect(false, false);
    WiFi.begin(credential.ssid, credential.password);
    Serial.printf("WiFi: trying network %u/%u (%s)\n",
                  static_cast<unsigned>(wifiNetworkIndex_ + 1),
                  static_cast<unsigned>(kWifiCredentialCount),
                  credential.ssid);
    configTzTime(kTimeZone, kNtpServerOne, kNtpServerTwo);
    wifiConfigured_ = true;
    lastWifiAttempt_ = millis();
}

void AstraGlassServices::stopWifi() {
    cameraWebServer_.stop();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    wifiConfigured_ = false;
    wifiNetworkIndex_ = 0;
}

void AstraGlassServices::updateWifi() {
    if (!wifiEnabled_) {
        if (wifiConfigured_ || cameraWebServer_.isRunning()) {
            stopWifi();
        }
        return;
    }

    if (!wifiConfigured_) {
        startWifi();
    }

    if (WiFi.status() == WL_CONNECTED) {
        if (gesturePageActive_ || !cameraStreamPageActive_) {
            if (cameraWebServer_.isRunning()) {
                cameraWebServer_.stop();
            }
            return;
        }
        if (!cameraWebServer_.isRunning()) {
            if (cameraWebServer_.start()) {
                printCameraStreamUrls();
            }
        }
        return;
    }

    if (cameraWebServer_.isRunning()) {
        cameraWebServer_.stop();
    }

    // ESP-DL needs the same internal heap that the S3 PHY wakeup path uses.
    // Defer reconnects while either gesture page owns the camera/model memory.
    if ((gesturePageActive_ || gestureControlMode_) ||
        millis() - lastWifiAttempt_ < kWifiRetryIntervalMs) {
        return;
    }

    wifiNetworkIndex_ = static_cast<std::uint8_t>(
        (wifiNetworkIndex_ + 1) % kWifiCredentialCount);
    startWifi();
}

void AstraGlassServices::printCameraStreamUrls() const {
    const String ip = WiFi.localIP().toString();
    Serial.printf("Camera web: http://%s/\n", ip.c_str());
    Serial.printf("Camera stream: http://%s:81/stream\n", ip.c_str());
}

const char *AstraGlassServices::currentWifiSsid() const {
    return kWifiCredentials[wifiNetworkIndex_ % kWifiCredentialCount].ssid;
}

void AstraGlassServices::updateGesture3D() {
    const bool gestureRequested = gesturePageActive_ || gestureControlMode_;
    if (cameraStreamPageActive_ || !gestureRequested) {
        if (gesture3D_.isActive()) {
            gesture3D_.stop();
        }
        return;
    }

    if (!gestureEnabled_) {
        if (gesture3D_.isActive()) {
            gesture3D_.stop();
        }
        return;
    }

    if (!gesture3D_.isActive()) {
        if (!startGesture3D()) {
            return;
        }
    }
    gesture3D_.update();

    if (gestureControlMode_) {
        AstraGestureDigit digit = astra_gesture::Digit::None;
        if (gesture3D_.takeNewDigit(digit)) {
            const astra_gesture_control::Action action =
                astra_gesture_control::actionFor(digit);
            if (action != astra_gesture_control::Action::None) {
                pendingGestureAction_ = action;
            }
        }
    }
}

void AstraGlassServices::updateBattery() {
    const std::uint32_t now = millis();
    if (now - lastBatteryUpdate_ < 1000) return;
    lastBatteryUpdate_ = now;
    batteryVoltage_ = glass_.getBattVoltage();
    batteryPercent_ = glass_.getBatteryPercent();
}

void AstraGlassServices::updateAudioLevels() {
    const std::uint32_t now = millis();
    if (!microphoneActive_ || now - lastAudioUpdate_ < 200) return;
    lastAudioUpdate_ = now;
    glass_.getAudioLevels(&audioLeft_, &audioRight_);
}

void AstraGlassServices::updateRtttl() {
    if (glass_.i2sRtttl == nullptr || !glass_.i2sRtttl->isRunning()) return;
    if (!glass_.i2sRtttl->loop()) glass_.i2sRtttl->stop();
}

void AstraGlassServices::loadPreferences() {
    const astra_settings::State defaults = astra_settings::defaultState();
    wifiEnabled_ = defaults.wifiEnabled;
    gestureEnabled_ = defaults.gestureEnabled;
    language_ = defaults.language;

    Preferences preferences;
    if (preferences.begin(astra_settings::kPreferencesNamespace, true)) {
        displayOffsetX_ = preferences.getShort("display_x", 0);
        displayOffsetY_ = preferences.getShort("display_y", 0);
        brightness_ = preferences.getUChar("brightness", 255);
        wifiEnabled_ = preferences.getBool(astra_settings::kWifiEnabledKey,
                                           defaults.wifiEnabled);
        gestureEnabled_ = preferences.getBool(astra_settings::kGestureEnabledKey,
                                              defaults.gestureEnabled);
        const std::uint8_t storedLanguage = preferences.getUChar(
            astra_settings::kLanguageKey,
            static_cast<std::uint8_t>(defaults.language));
        language_ = storedLanguage == static_cast<std::uint8_t>(astra_settings::Language::English)
                        ? astra_settings::Language::English
                        : astra_settings::Language::Chinese;
        preferences.getBytes("lora", &loraSettings_, sizeof(loraSettings_));
        preferences.end();
    }

    displayOffsetX_ = std::max(kMinimumDisplayOffset,
                               std::min(displayOffsetX_, kMaximumDisplayOffset));
    displayOffsetY_ = std::max(kMinimumDisplayOffset,
                               std::min(displayOffsetY_, kMaximumDisplayOffset));
    languageChinese_ = language_ == astra_settings::Language::Chinese;
    languageEnglish_ = language_ == astra_settings::Language::English;
    hal_.setDisplayCalibration(displayOffsetX_, displayOffsetY_);
    glass_.setBrightness(brightness_);
}

void AstraGlassServices::savePreferences() {
    Preferences preferences;
    if (!preferences.begin(astra_settings::kPreferencesNamespace, false)) return;
    preferences.putShort("display_x", displayOffsetX_);
    preferences.putShort("display_y", displayOffsetY_);
    preferences.putUChar("brightness", brightness_);
    preferences.putBool(astra_settings::kWifiEnabledKey, wifiEnabled_);
    preferences.putBool(astra_settings::kGestureEnabledKey, gestureEnabled_);
    preferences.putUChar(astra_settings::kLanguageKey,
                         static_cast<std::uint8_t>(language_));
    preferences.putBytes("lora", &loraSettings_, sizeof(loraSettings_));
    preferences.end();
}

bool &AstraGlassServices::wifiEnabledSetting() {
    return wifiEnabled_;
}

bool &AstraGlassServices::gestureEnabledSetting() {
    return gestureEnabled_;
}

bool &AstraGlassServices::languageChineseSetting() {
    return languageChinese_;
}

bool &AstraGlassServices::languageEnglishSetting() {
    return languageEnglish_;
}

bool AstraGlassServices::wifiEnabled() const {
    return wifiEnabled_;
}

bool AstraGlassServices::gestureEnabled() const {
    return gestureEnabled_;
}

bool AstraGlassServices::isEnglish() const {
    return language_ == astra_settings::Language::English;
}

astra_settings::Language AstraGlassServices::language() const {
    return language_;
}

const char *AstraGlassServices::setLanguage(astra_settings::Language language) {
    language_ = language == astra_settings::Language::English
                    ? astra_settings::Language::English
                    : astra_settings::Language::Chinese;
    languageChinese_ = language_ == astra_settings::Language::Chinese;
    languageEnglish_ = language_ == astra_settings::Language::English;
    savePreferences();
    return text(language_ == astra_settings::Language::English
                    ? astra_language::TextId::LanguageSelectedEnglish
                    : astra_language::TextId::LanguageSelectedChinese);
}

const char *AstraGlassServices::text(astra_language::TextId id) const {
    return astra_language::text(id, language_);
}

const char *AstraGlassServices::toggleWifi() {
    wifiEnabled_ = !wifiEnabled_;
    savePreferences();
    if (wifiEnabled_) {
        startWifi();
        return text(astra_language::TextId::WifiEnabledAction);
    }
    stopWifi();
    return text(astra_language::TextId::WifiDisabledAction);
}

const char *AstraGlassServices::toggleGesture() {
    gestureEnabled_ = !gestureEnabled_;
    savePreferences();
    if (!gestureEnabled_ && gesture3D_.isActive()) {
        gesture3D_.stop();
    }
    if (!gestureEnabled_) {
        gestureControlMode_ = false;
        gestureControlPageActive_ = false;
        pendingGestureAction_ = astra_gesture_control::Action::None;
    }
    if (gestureEnabled_ && gesturePageActive_ && !gesture3D_.isActive()) {
        startGesture3D();
    }
    return text(gestureEnabled_ ? astra_language::TextId::GestureEnabledAction
                                : astra_language::TextId::GestureDisabledAction);
}

void AstraGlassServices::enterGesture3D() {
    cameraStreamPageActive_ = false;
    cameraWebServer_.stop();
    gestureControlMode_ = false;
    gestureControlPageActive_ = false;
    pendingGestureAction_ = astra_gesture_control::Action::None;
    gesturePageActive_ = true;
    if (gestureEnabled_) {
        startGesture3D();
    }
}

void AstraGlassServices::exitGesture3D() {
    gesturePageActive_ = false;
    if (!gestureControlMode_) {
        gesture3D_.stop();
    }
}

void AstraGlassServices::enterGestureControl() {
    cameraStreamPageActive_ = false;
    cameraWebServer_.stop();
    gesturePageActive_ = false;
    gestureControlPageActive_ = true;
    gestureControlMode_ = true;
    pendingGestureAction_ = astra_gesture_control::Action::None;
    if (gestureEnabled_) {
        startGesture3D();
    }
}

void AstraGlassServices::exitGestureControl() {
    gestureControlPageActive_ = false;
}

bool AstraGlassServices::takeGestureAction(astra_gesture_control::Action &action) {
    if (pendingGestureAction_ == astra_gesture_control::Action::None) {
        return false;
    }
    action = pendingGestureAction_;
    pendingGestureAction_ = astra_gesture_control::Action::None;
    return true;
}

void AstraGlassServices::toggleRadioPlayback() {
    radioPlayer_.togglePlayback();
}

void AstraGlassServices::enterCameraStream() {
    gesturePageActive_ = false;
    gestureControlPageActive_ = false;
    gestureControlMode_ = false;
    pendingGestureAction_ = astra_gesture_control::Action::None;
    gesture3D_.stop();
    cameraStreamPageActive_ = true;
    cameraWebServer_.stop();
}

void AstraGlassServices::exitCameraStream() {
    cameraStreamPageActive_ = false;
    cameraWebServer_.stop();
}

bool AstraGlassServices::gesture3DDisplayActive() const {
    return gesturePageActive_ && !gestureControlMode_ && gesture3D_.isActive() &&
           gesture3D_.currentDigit() != astra_gesture::Digit::None;
}

bool AstraGlassServices::startGesture3D() {
    if (gesture3D_.isActive()) {
        return true;
    }

    // The ESP-DL detector needs one large contiguous PSRAM workspace. Keep
    // Stop the HTTP server before handing the shared camera to Gesture3D.
    cameraWebServer_.stop();
    if (gesture3D_.start()) {
        return true;
    }

    return false;
}

bool AstraGlassServices::configureRadio() {
    if (!radioReady_) return false;

    radio_.standby();
    if (radio_.setFrequency(loraSettings_.frequency) != RADIOLIB_ERR_NONE) return false;
    if (radio_.setBandwidth(loraSettings_.bandwidth) != RADIOLIB_ERR_NONE) return false;
    if (radio_.setSpreadingFactor(loraSettings_.spreadingFactor) != RADIOLIB_ERR_NONE) {
        return false;
    }
    if (radio_.setCodingRate(loraSettings_.codingRate) != RADIOLIB_ERR_NONE) return false;
    if (radio_.setSyncWord(loraSettings_.syncWord) != RADIOLIB_ERR_NONE) return false;
    if (radio_.setOutputPower(loraSettings_.txPower) != RADIOLIB_ERR_NONE) return false;
    if (radio_.setCurrentLimit(140) != RADIOLIB_ERR_NONE) return false;
    if (radio_.setPreambleLength(loraSettings_.preambleLength) != RADIOLIB_ERR_NONE) {
        return false;
    }
    radio_.setCRC(false);
    radio_.setDio1Action(radioEventThunk);
    return true;
}

void AstraGlassServices::radioEventThunk() {
    if (activeRadioService_ != nullptr) activeRadioService_->radioEvent_ = true;
}

void AstraGlassServices::pollLoRaReceive() {
    if (!radioEvent_) return;
    radioEvent_ = false;

    String received;
    radioState_ = radio_.readData(received);
    if (radioState_ == RADIOLIB_ERR_NONE) {
        lastReceived_ = asStdString(received);
        lastRssi_ = radio_.getRSSI();
        lastSnr_ = radio_.getSNR();
    }
    radio_.startReceive();
}

void AstraGlassServices::startMicrophone() {
    if (microphoneActive_) return;
    radioPlayer_.setPlaybackEnabled(false);
    stopRtttl();
    if (glass_.audioOut != nullptr) glass_.audioOut->stop();
    microphoneReady_ = glass_.initI2S();
    microphoneActive_ = true;
}

void AstraGlassServices::stopMicrophone() {
    if (!microphoneActive_) return;
    glass_.deinitI2S();
    microphoneActive_ = false;
    microphoneReady_ = false;
    audioLeft_ = 0;
    audioRight_ = 0;
}

void AstraGlassServices::playRtttl(const char *rtttl) {
    if (rtttl == nullptr || glass_.audioOut == nullptr || glass_.rtttlFile == nullptr ||
        glass_.i2sRtttl == nullptr) {
        return;
    }

    radioPlayer_.setPlaybackEnabled(false);
    stopMicrophone();
    if (glass_.i2sRtttl->isRunning()) glass_.i2sRtttl->stop();
    if (glass_.rtttlFile->open(rtttl, std::strlen(rtttl))) {
        glass_.i2sRtttl->begin(glass_.rtttlFile, glass_.audioOut);
    }
}

void AstraGlassServices::stopRtttl() {
    if (glass_.i2sRtttl != nullptr && glass_.i2sRtttl->isRunning()) {
        glass_.i2sRtttl->stop();
    }
}

void AstraGlassServices::enterRadio() {
    stopMicrophone();
    stopRtttl();
}

void AstraGlassServices::exitRadio() {
    radioPlayer_.setPlaybackEnabled(false);
}

const char *AstraGlassServices::selectRadioStation(std::size_t index) {
    if (index >= radio_demo::kStationCount) {
        return text(astra_language::TextId::RadioStationError);
    }

    stopMicrophone();
    stopRtttl();
    return radioPlayer_.selectStation(index) ? nullptr :
        text(astra_language::TextId::RadioStationError);
}

void AstraGlassServices::increaseRadioVolume() {
    radioPlayer_.increaseVolume();
}

void AstraGlassServices::renderRadioDetail() {
    const radio_demo::RadioDemoStation *station = radioPlayer_.station();
    if (station == nullptr) return;

    HAL::setFont(u8g2_font_wqy12_t_gb2312);
    HAL::setDrawType(1);
    HAL::drawChinese(0, 8, text(astra_language::TextId::RadioTitle));

    char volume[8] = {};
    std::snprintf(volume, sizeof(volume), "%u%%",
                  static_cast<unsigned>(radioPlayer_.volume()));
    HAL::drawEnglish(98, 8, volume);
    HAL::drawHLine(0, 10, AstraPortSurface::kWidth);

    drawRadioText(makeRadioMarquee(station->name == nullptr ? "" : station->name), 2, 25);

    const char *streamTitle = radioPlayer_.streamTitle();
    if (streamTitle != nullptr && streamTitle[0] != '\0') {
        drawRadioText(makeRadioMarquee(streamTitle), 2, 39);
    }

    const radio_demo::RadioDemoState state = radioPlayer_.state();
    const bool radioAnimation = state == radio_demo::RadioDemoState::Playing ||
                                state == radio_demo::RadioDemoState::Buffering ||
                                state == radio_demo::RadioDemoState::Connecting ||
                                state == radio_demo::RadioDemoState::Retrying;
    const std::uint32_t radioAnimationPhase = millis() / 160;
    drawRadioEqualizer(radioAnimationPhase, radioAnimation);
    drawRadioText(radioStateLabel(state, language_), 48, 55);
}

void AstraGlassServices::startLoRaTransmitPacket() {
    char payload[24] = {};
    std::snprintf(payload, sizeof(payload), "Hello %lu",
                  static_cast<unsigned long>(transmitCount_ + 1));
    transmitPayload_ = payload;

    radioEvent_ = false;
    radioState_ = radio_.startTransmit(transmitPayload_.c_str());
    transmitInFlight_ = radioState_ == RADIOLIB_ERR_NONE;
    if (transmitInFlight_) {
        Serial.printf("LoRa TX: %s\n", transmitPayload_.c_str());
        return;
    }

    transmitActive_ = false;
    radio_.sleep(true);
}

void AstraGlassServices::pollLoRaTransmit() {
    if (!transmitActive_) return;

    if (transmitInFlight_) {
        if (!radioEvent_) return;
        radioEvent_ = false;
        radioState_ = radio_.finishTransmit();
        transmitInFlight_ = false;
        if (radioState_ != RADIOLIB_ERR_NONE) {
            Serial.printf("LoRa TX error: %d\n", radioState_);
            transmitActive_ = false;
            radio_.sleep(true);
            return;
        }

        ++transmitCount_;
        Serial.printf("LoRa TX done #%lu\n",
                      static_cast<unsigned long>(transmitCount_));
        nextTransmitAt_ = millis() + kLoRaTransmitIntervalMs;
    }

    const std::uint32_t now = millis();
    if (transmitInFlight_ || static_cast<std::int32_t>(now - nextTransmitAt_) < 0) {
        return;
    }
    startLoRaTransmitPacket();
}

void AstraGlassServices::startLoRaTransmit() {
    if (!radioReady_ || transmitActive_) return;
    if (receiveActive_) stopLoRaReceive();
    if (!configureRadio()) {
        radioState_ = RADIOLIB_ERR_UNKNOWN;
        return;
    }

    transmitActive_ = true;
    transmitInFlight_ = false;
    transmitCount_ = 0;
    nextTransmitAt_ = millis();
    startLoRaTransmitPacket();
}

void AstraGlassServices::stopLoRaTransmit() {
    if (!transmitActive_ && !transmitInFlight_) return;

    transmitActive_ = false;
    transmitInFlight_ = false;
    radioEvent_ = false;
    radio_.standby();
    radio_.sleep(true);
}

void AstraGlassServices::startLoRaReceive() {
    if (!radioReady_ || receiveActive_) return;
    if (transmitActive_ || transmitInFlight_) stopLoRaTransmit();
    receiveActive_ = false;
    radioEvent_ = false;
    if (!configureRadio()) {
        radioState_ = RADIOLIB_ERR_UNKNOWN;
        return;
    }
    radioState_ = radio_.startReceive();
    receiveActive_ = radioState_ == RADIOLIB_ERR_NONE;
}

void AstraGlassServices::stopLoRaReceive() {
    if (!receiveActive_) return;
    radio_.standby();
    radio_.sleep(true);
    receiveActive_ = false;
    radioEvent_ = false;
}

void AstraGlassServices::scanWifiNow() {
    scanCount_ = 0;
    const int networkCount = WiFi.scanNetworks(false, true);
    if (networkCount <= 0) {
        WiFi.scanDelete();
        return;
    }

    const int count = std::min(networkCount, 4);
    for (int index = 0; index < count; ++index) {
        scanNames_[index] = WiFi.SSID(index);
        scanRssi_[index] = WiFi.RSSI(index);
    }
    scanCount_ = static_cast<std::uint8_t>(count);
    WiFi.scanDelete();
}

const char *AstraGlassServices::speakerTest() {
    glass_.tone();
    return text(astra_language::TextId::SpeakerTestDone);
}

const char *AstraGlassServices::nudgeDisplay(std::int16_t deltaX, std::int16_t deltaY) {
    displayOffsetX_ = std::max(kMinimumDisplayOffset,
                               std::min<std::int16_t>(displayOffsetX_ + deltaX,
                                                      kMaximumDisplayOffset));
    displayOffsetY_ = std::max(kMinimumDisplayOffset,
                               std::min<std::int16_t>(displayOffsetY_ + deltaY,
                                                      kMaximumDisplayOffset));
    hal_.setDisplayCalibration(displayOffsetX_, displayOffsetY_);
    return text(astra_language::TextId::DisplayMoved);
}

const char *AstraGlassServices::saveDisplayCalibration() {
    savePreferences();
    return text(astra_language::TextId::DisplaySaved);
}

const char *AstraGlassServices::setBrightnessLevel(std::uint8_t brightness) {
    brightness_ = brightness;
    glass_.setBrightness(brightness_);
    savePreferences();
    return text(astra_language::TextId::BrightnessSet);
}

const char *AstraGlassServices::sleepNow() {
    rtc_gpio_pullup_en(GPIO_NUM_1);
    rtc_gpio_pulldown_dis(GPIO_NUM_1);
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_1, 1);
    glass_.sleep();
    esp_deep_sleep_start();
    return text(astra_language::TextId::Sleeping);
}

void AstraGlassServices::drawHeader(const char *title) const {
    HAL::setFont(u8g2_font_wqy12_t_gb2312);
    HAL::setDrawType(1);
    HAL::drawChinese(0, 8, title);
    HAL::drawHLine(0, 10, AstraPortSurface::kWidth);
}

void AstraGlassServices::drawLine(const std::string &text, std::int16_t baseline) const {
    HAL::setFont(u8g2_font_wqy12_t_gb2312);
    HAL::setDrawType(1);
    HAL::drawChinese(0, baseline, shorten(text, 21));
}

void AstraGlassServices::drawCentered(const std::string &text, std::int16_t baseline) const {
    HAL::setFont(u8g2_font_wqy12_t_gb2312);
    std::string shortened = shorten(text, 21);
    const int width = HAL::getFontWidth(shortened);
    HAL::setDrawType(1);
    HAL::drawChinese((AstraPortSurface::kWidth - width) / 2, baseline, shortened);
}

void AstraGlassServices::drawBar(std::int16_t y, int value, int maximum) const {
    const int width = 100;
    const int clamped = maximum <= 0 ? 0 : std::max(0, std::min(value, maximum));
    const int filled = maximum <= 0 ? 0 : (clamped * width) / maximum;
    HAL::setDrawType(1);
    HAL::drawFrame(14, y, width, 8);
    if (filled > 2) {
        HAL::drawBox(15, y + 1, filled - 2, 6);
    }
}

std::string AstraGlassServices::shorten(const std::string &text, std::size_t maximum) {
    std::size_t offset = 0;
    std::size_t count = 0;
    while (offset < text.size() && count < maximum) {
        const unsigned char first = static_cast<unsigned char>(text[offset]);
        std::size_t length = 1;
        if ((first & 0xE0U) == 0xC0U) {
            length = 2;
        } else if ((first & 0xF0U) == 0xE0U) {
            length = 3;
        } else if ((first & 0xF8U) == 0xF0U) {
            length = 4;
        }
        offset = std::min(offset + length, text.size());
        ++count;
    }
    return offset == text.size() ? text : text.substr(0, offset);
}

void AstraGlassServices::renderStatusBar() const {
    hal_.setContentOffsetY(0);

    const astra_status_bar::NetworkState networkState =
        !wifiEnabled_ ? astra_status_bar::NetworkState::Disabled
                       : WiFi.status() == WL_CONNECTED
                           ? astra_status_bar::NetworkState::Connected
                           : astra_status_bar::NetworkState::Connecting;

    time_t now = time(nullptr);
    struct tm timeInfo = {};
    localtime_r(&now, &timeInfo);
    std::string clockText = timeInfo.tm_year >= 120
                                ? astra_status_bar::formatClock(timeInfo.tm_hour,
                                                                timeInfo.tm_min)
                                : "--:--";

    const int batteryPercent = astra_status_bar::clampBattery(batteryPercent_);
    char batteryValue[8] = {};
    std::snprintf(batteryValue, sizeof(batteryValue), "%d%%", batteryPercent);
    std::string batteryText = batteryValue;

    HAL::setDrawType(0);
    HAL::drawBox(0, 0, AstraPortSurface::kWidth, astra_status_bar::kHeight);
    HAL::setFont(u8g2_font_5x7_tr);
    HAL::setDrawType(1);

    constexpr std::int16_t kWifiX = 2;
    constexpr std::int16_t kWifiY = 0;
    for (int row = 0; row < astra_status_bar::kWifiIconHeight; ++row) {
        const std::uint16_t mask = astra_status_bar::wifiIconRow(networkState, row);
        for (int column = 0; column < astra_status_bar::kWifiIconWidth; ++column) {
            if ((mask & (static_cast<std::uint16_t>(1U) <<
                         (astra_status_bar::kWifiIconWidth - 1 - column))) != 0) {
                HAL::drawPixel(kWifiX + column, kWifiY + row);
            }
        }
    }

    const int clockWidth = HAL::getFontWidth(clockText);
    HAL::drawEnglish((AstraPortSurface::kWidth - clockWidth) / 2, 7, clockText);

    const int batteryWidth = HAL::getFontWidth(batteryText);
    const int batteryTextX = AstraPortSurface::kWidth - batteryWidth - 6;
    const int batteryX = batteryTextX - 15;
    HAL::drawFrame(batteryX, 2, 12, 6);
    HAL::drawBox(batteryX + 12, 4, 2, 2);
    const int fillWidth = (batteryPercent * 10) / 100;
    if (fillWidth > 0) {
        HAL::drawBox(batteryX + 1, 3, fillWidth, 4);
    }
    HAL::drawEnglish(batteryTextX, 7, batteryText);
    HAL::drawHLine(0, astra_status_bar::kHeight - 1, AstraPortSurface::kWidth);

    // Keep the main UI font active for the next frame.
    HAL::setFont(u8g2_font_wqy12_t_gb2312);
    hal_.setContentOffsetY(astra_status_bar::kContentTopInset);
}

void AstraGlassServices::renderCameraStatus() {
    drawHeader(text(astra_language::TextId::CameraStatus));
    drawLine(text(glass_.isCameraDetected() ? astra_language::TextId::CameraOnline
                                            : astra_language::TextId::CameraOffline),
             25);
    drawLine(text(astra_language::TextId::FormatJpeg), 39);
    drawLine(text(astra_language::TextId::FrameQvga), 53);
}

void AstraGlassServices::renderGesture3DStatus() {
    if (!gestureEnabled_) {
        drawCentered(text(astra_language::TextId::EnableGestureFirst), 36);
        return;
    }
    if (gesture3D_.currentDigit() == astra_gesture::Digit::None) {
        drawCentered(text(astra_language::TextId::ShowGesture1To5), 36);
        return;
    }
}

void AstraGlassServices::renderCameraStreamStatus() {
    drawHeader(text(astra_language::TextId::CameraStream));
    if (!wifiEnabled_) {
        drawLine(text(astra_language::TextId::WifiDisabled), 25);
        drawLine(text(astra_language::TextId::EnableWifiFirst), 43);
        return;
    }
    if (WiFi.status() != WL_CONNECTED) {
        drawLine(text(astra_language::TextId::WifiConnecting), 25);
        drawLine(std::string(text(astra_language::TextId::Ssid)) + currentWifiSsid(), 43);
        return;
    }

    const std::string ip = asStdString(WiFi.localIP().toString());
    drawLine(text(astra_language::TextId::Web), 19);
    drawLine(ip + "/", 33);
    drawLine(text(astra_language::TextId::Stream), 45);
    drawLine(text(cameraWebServer_.isRunning() ? astra_language::TextId::ServerOnline
                                                : astra_language::TextId::ServerStarting),
             53);
}

void AstraGlassServices::renderGestureControlStatus() {
    drawHeader(text(astra_language::TextId::GestureControl));
    if (!gestureEnabled_) {
        drawCentered(text(astra_language::TextId::EnableGestureFirst), 36);
        return;
    }
    drawLine(text(astra_language::TextId::GestureOnePrevious), 15);
    drawLine(text(astra_language::TextId::GestureTwoNext), 24);
    drawLine(text(astra_language::TextId::GestureThreeOpen), 33);
    drawLine(text(astra_language::TextId::GestureFourClose), 42);
    drawLine(text(astra_language::TextId::GestureFivePlayback), 51);
}

void AstraGlassServices::renderGesture3D() {
    if (gestureControlMode_ || !gesture3D_.isActive()) {
        return;
    }
    gesture3D_.render();
    if (gesture3D_.currentDigit() == astra_gesture::Digit::None) {
        HAL::canvasUpdate();
    }
}

void AstraGlassServices::renderMicrophone() {
    drawHeader(text(astra_language::TextId::MicrophoneVolume));
    drawLine(text(microphoneReady_ ? astra_language::TextId::MicrophoneReady
                                   : astra_language::TextId::MicrophoneOffline),
             19);
    char levels[32] = {};
    std::snprintf(levels, sizeof(levels), "L:%5d R:%5d", audioLeft_, audioRight_);
    drawCentered(levels, 33);
    drawBar(39, audioLeft_, 16384);
    drawBar(53, audioRight_, 16384);
}

void AstraGlassServices::renderInputTest() {
    drawHeader(text(astra_language::TextId::InputTest));
    drawLine(text(digitalRead(BOARD_BOOT_PIN) == LOW ? astra_language::TextId::BootPressed
                                                      : astra_language::TextId::BootIdle),
             24);
    drawLine(text(digitalRead(BOARD_TOUCH_BUTTON) == HIGH ? astra_language::TextId::TouchPressed
                                                          : astra_language::TextId::TouchIdle),
             40);
    drawLine(text(astra_language::TextId::Gpio0Gpio1), 56);
}

void AstraGlassServices::renderLoRaTransmit() {
    drawHeader(text(astra_language::TextId::ContinuousTransmit));
    if (!radioReady_) {
        drawLine(text(astra_language::TextId::RadioOffline), 25);
        return;
    }

    drawLine(text(transmitActive_ ? astra_language::TextId::LoRaTransmitActive
                                  : astra_language::TextId::LoRaTransmitStopped),
             20);
    drawLine(std::string("TX: ") + transmitPayload_, 36);

    char count[32] = {};
    std::snprintf(count, sizeof(count), "COUNT: %lu",
                  static_cast<unsigned long>(transmitCount_));
    drawLine(count, 52);
}

void AstraGlassServices::renderLoRaReceive() {
    drawHeader(text(astra_language::TextId::ReceiveMonitor));
    if (!radioReady_) {
        drawLine(text(astra_language::TextId::RadioOffline), 25);
        return;
    }
    drawLine(text(receiveActive_ ? astra_language::TextId::RadioListening
                                 : astra_language::TextId::RadioStopped),
             19);
    drawLine("RX: " + shorten(lastReceived_, 18), 33);
    char signal[32] = {};
    std::snprintf(signal, sizeof(signal), "RSSI:%6.1f SNR:%5.1f", lastRssi_, lastSnr_);
    drawLine(signal, 49);
}

void AstraGlassServices::renderLoRaParameters() {
    drawHeader(text(astra_language::TextId::LoRaParameters));
    char line[32] = {};
    std::snprintf(line, sizeof(line), "F:%.1f BW:%.1f", loraSettings_.frequency,
                  loraSettings_.bandwidth);
    drawLine(line, 22);
    std::snprintf(line, sizeof(line), "SF:%u CR:4/%u",
                  static_cast<unsigned>(loraSettings_.spreadingFactor),
                  static_cast<unsigned>(loraSettings_.codingRate));
    drawLine(line, 38);
    std::snprintf(line, sizeof(line), "PWR:%udBm SW:0x%02X",
                  static_cast<unsigned>(loraSettings_.txPower),
                  static_cast<unsigned>(loraSettings_.syncWord));
    drawLine(line, 54);
}

void AstraGlassServices::renderWirelessStatus() {
    drawHeader(text(astra_language::TextId::WirelessStatus));
    drawLine(text(radioReady_ ? astra_language::TextId::RadioReady
                              : astra_language::TextId::RadioOffline),
             24);
    drawLine(text(receiveActive_ ? astra_language::TextId::ModeReceive
                                 : astra_language::TextId::RadioStandby),
             40);
    char state[32] = {};
    std::snprintf(state, sizeof(state), "State: %d", radioState_);
    drawLine(state, 56);
}

void AstraGlassServices::renderWifiStatus() {
    drawHeader(text(astra_language::TextId::WifiStatus));
    if (!wifiEnabled_) {
        drawLine(text(astra_language::TextId::WifiDisabled), 25);
        return;
    }
    if (WiFi.status() != WL_CONNECTED) {
        drawLine(text(astra_language::TextId::WifiConnecting), 25);
        drawLine(std::string(text(astra_language::TextId::Ssid)) + currentWifiSsid(), 43);
        return;
    }
    drawLine(text(astra_language::TextId::WifiConnected), 19);
    drawLine(std::string(text(astra_language::TextId::Ssid)) +
                 shorten(asStdString(WiFi.SSID()), 15),
             35);
    drawLine("IP: " + asStdString(WiFi.localIP().toString()), 51);
}

void AstraGlassServices::renderWifiScan() {
    drawHeader(text(astra_language::TextId::WifiScan));
    if (scanCount_ == 0) {
        drawCentered(text(astra_language::TextId::NoNetworks), 36);
        return;
    }
    for (std::uint8_t index = 0; index < scanCount_ && index < 3; ++index) {
        char line[32] = {};
        std::snprintf(line, sizeof(line), "%u:%s %ddBm", static_cast<unsigned>(index + 1),
                      scanNames_[index].c_str(), scanRssi_[index]);
        drawLine(line, 24 + index * 14);
    }
}

void AstraGlassServices::renderTime() {
    drawHeader(text(astra_language::TextId::TimeNtp));
    time_t now = time(nullptr);
    struct tm timeInfo = {};
    localtime_r(&now, &timeInfo);
    char clockText[32] = {};
    std::snprintf(clockText, sizeof(clockText), "%02d:%02d:%02d", timeInfo.tm_hour,
                  timeInfo.tm_min, timeInfo.tm_sec);
    drawCentered(clockText, 29);
    char dateText[32] = {};
    std::snprintf(dateText, sizeof(dateText), "%04d-%02d-%02d", timeInfo.tm_year + 1900,
                  timeInfo.tm_mon + 1, timeInfo.tm_mday);
    drawCentered(dateText, 47);
}

void AstraGlassServices::renderBattery() {
    drawHeader(text(astra_language::TextId::Battery));
    char value[20] = {};
    std::snprintf(value, sizeof(value), "%.2fV", batteryVoltage_ / 1000.0f);
    drawCentered(std::string(text(astra_language::TextId::Voltage)) + value, 27);
    std::snprintf(value, sizeof(value), "%d%%", batteryPercent_);
    drawCentered(std::string(text(astra_language::TextId::Charge)) + value, 45);
    drawBar(51, batteryPercent_, 100);
}

void AstraGlassServices::renderDiagnostics() {
    drawHeader(text(astra_language::TextId::FactoryDiagnostics));
    drawLine(text(glass_.isCameraDetected() ? astra_language::TextId::CameraOk
                                            : astra_language::TextId::CameraFail),
             20);
    drawLine(text(microphoneReady_ ? astra_language::TextId::MicActive
                                   : astra_language::TextId::MicIdle),
             34);
    drawLine(text(radioReady_ ? astra_language::TextId::LoRaOk
                              : astra_language::TextId::LoRaFail),
             48);
    drawLine(text(WiFi.status() == WL_CONNECTED ? astra_language::TextId::WifiConnected
                                                : astra_language::TextId::WifiOffline),
              62);
}
