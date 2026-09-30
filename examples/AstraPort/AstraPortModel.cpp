#include "AstraPortModel.h"

#include "AstraDinoJumpController.h"
#include "AstraGlassServices.h"
#include "AstraLanguage.h"
#include "AstraPortIcons.h"
#include "RadioDemoConfig.h"

#include <cstddef>
#include <string>

namespace {

using TextId = astra_language::TextId;

bool menuUsesEnglish(void *context) {
    const auto *services = static_cast<const AstraGlassServices *>(context);
    return services != nullptr && services->isEnglish();
}

void localizeMenu(astra::Menu *menu, TextId id, AstraGlassServices *services) {
    if (menu == nullptr || services == nullptr) return;
    menu->setLocalizedTitle(astra_language::text(id, astra_settings::Language::Chinese),
                            astra_language::text(id, astra_settings::Language::English),
                            menuUsesEnglish,
                            services);
}

astra::List *localizedList(TextId id, AstraGlassServices *services) {
    auto *item = new astra::List(astra_language::text(id, astra_settings::Language::Chinese));
    localizeMenu(item, id, services);
    return item;
}

astra::List *localizedList(TextId id,
                           AstraGlassServices *services,
                           const std::vector<unsigned char> &icon) {
    auto *item = new astra::List(astra_language::text(id, astra_settings::Language::Chinese),
                                 icon);
    localizeMenu(item, id, services);
    return item;
}

astra::Tile *localizedTile(TextId id,
                           AstraGlassServices *services,
                           const std::vector<unsigned char> &icon) {
    auto *item = new astra::Tile(astra_language::text(id, astra_settings::Language::Chinese),
                                 icon);
    localizeMenu(item, id, services);
    return item;
}

class AstraStatusPage final : public astra::List {
public:
    using RenderCallback = void (*)(void *context);

    AstraStatusPage(const std::string &title,
                    void *context,
                    RenderCallback render,
                    astra::Menu::LifecycleCallback enter = nullptr,
                    astra::Menu::LifecycleCallback exit = nullptr)
        : astra::List(title), render_(render), context_(context) {
        setEnterCallback(enter, context);
        setExitCallback(exit, context);
    }

    AstraStatusPage(TextId titleId,
                    AstraGlassServices *languageContext,
                    RenderCallback render,
                    astra::Menu::LifecycleCallback enter = nullptr,
                    astra::Menu::LifecycleCallback exit = nullptr)
        : AstraStatusPage(astra_language::text(titleId, astra_settings::Language::Chinese),
                          languageContext,
                          render,
                          enter,
                          exit) {
        localizeMenu(this, titleId, languageContext);
    }

    [[nodiscard]] std::string getType() const override {
        return "Status";
    }

    bool canOpenWithoutChildren() const override {
        return true;
    }

    void render(const std::vector<float> &camera) override {
        (void)camera;
        if (render_ != nullptr) render_(context_);
    }

private:
    RenderCallback render_ = nullptr;
    void *context_ = nullptr;
};

void renderCameraStatus(void *context) {
    static_cast<AstraGlassServices *>(context)->renderCameraStatus();
}

void renderGesture3DStatus(void *context) {
    static_cast<AstraGlassServices *>(context)->renderGesture3DStatus();
}

void renderGestureControlStatus(void *context) {
    static_cast<AstraGlassServices *>(context)->renderGestureControlStatus();
}

void renderCameraStreamStatus(void *context) {
    static_cast<AstraGlassServices *>(context)->renderCameraStreamStatus();
}

void enterGesture3D(void *context) {
    static_cast<AstraGlassServices *>(context)->enterGesture3D();
}

void exitGesture3D(void *context) {
    static_cast<AstraGlassServices *>(context)->exitGesture3D();
}

void enterGestureControl(void *context) {
    static_cast<AstraGlassServices *>(context)->enterGestureControl();
}

void exitGestureControl(void *context) {
    static_cast<AstraGlassServices *>(context)->exitGestureControl();
}

void enterCameraStream(void *context) {
    static_cast<AstraGlassServices *>(context)->enterCameraStream();
}

void exitCameraStream(void *context) {
    static_cast<AstraGlassServices *>(context)->exitCameraStream();
}

void renderMicrophone(void *context) {
    static_cast<AstraGlassServices *>(context)->renderMicrophone();
}

void renderInputTest(void *context) {
    static_cast<AstraGlassServices *>(context)->renderInputTest();
}

void renderLoRaReceive(void *context) {
    static_cast<AstraGlassServices *>(context)->renderLoRaReceive();
}

void renderLoRaTransmit(void *context) {
    static_cast<AstraGlassServices *>(context)->renderLoRaTransmit();
}

void renderLoRaParameters(void *context) {
    static_cast<AstraGlassServices *>(context)->renderLoRaParameters();
}

void renderWirelessStatus(void *context) {
    static_cast<AstraGlassServices *>(context)->renderWirelessStatus();
}

void renderWifiStatus(void *context) {
    static_cast<AstraGlassServices *>(context)->renderWifiStatus();
}

void renderWifiScan(void *context) {
    static_cast<AstraGlassServices *>(context)->renderWifiScan();
}

void renderTime(void *context) {
    static_cast<AstraGlassServices *>(context)->renderTime();
}

void renderBattery(void *context) {
    static_cast<AstraGlassServices *>(context)->renderBattery();
}

void renderDiagnostics(void *context) {
    static_cast<AstraGlassServices *>(context)->renderDiagnostics();
}

void enterMicrophone(void *context) {
    static_cast<AstraGlassServices *>(context)->startMicrophone();
}

void exitMicrophone(void *context) {
    static_cast<AstraGlassServices *>(context)->stopMicrophone();
}

void enterRadio(void *context) {
    static_cast<AstraGlassServices *>(context)->enterRadio();
}

void exitRadio(void *context) {
    static_cast<AstraGlassServices *>(context)->exitRadio();
}

struct RadioStationContext {
    AstraGlassServices *services = nullptr;
    std::size_t index = 0;
};

void enterRadioStation(void *context) {
    auto *station = static_cast<RadioStationContext *>(context);
    if (station != nullptr && station->services != nullptr) {
        station->services->selectRadioStation(station->index);
    }
}

void renderRadioDetail(void *context) {
    auto *station = static_cast<RadioStationContext *>(context);
    if (station != nullptr && station->services != nullptr) {
        station->services->renderRadioDetail();
    }
}

bool radioDetailClick(void *context, unsigned char keyIndex) {
    auto *station = static_cast<RadioStationContext *>(context);
    if (station == nullptr || station->services == nullptr || keyIndex != 0) return false;
    station->services->increaseRadioVolume();
    return true;
}

void trimLastUtf8CodePoint(std::string &text) {
    if (text.empty()) return;
    std::size_t index = text.size() - 1;
    while (index > 0 && (static_cast<unsigned char>(text[index]) & 0xC0U) == 0x80U) {
        --index;
    }
    text.erase(index);
}

std::string fitRadioStationTitle(const char *title) {
    const std::string original = title == nullptr ? "Unknown station" : title;
    std::string label = original;
    const std::string suffix = "...";
    if (HAL::getFontWidth(label) <= 104) return label;

    while (!label.empty()) {
        std::string candidate = label + suffix;
        if (HAL::getFontWidth(candidate) <= 104) return candidate;
        trimLastUtf8CodePoint(label);
    }
    return suffix;
}

void enterLoRaReceive(void *context) {
    static_cast<AstraGlassServices *>(context)->startLoRaReceive();
}

void exitLoRaReceive(void *context) {
    static_cast<AstraGlassServices *>(context)->stopLoRaReceive();
}

void enterLoRaTransmit(void *context) {
    static_cast<AstraGlassServices *>(context)->startLoRaTransmit();
}

void exitLoRaTransmit(void *context) {
    static_cast<AstraGlassServices *>(context)->stopLoRaTransmit();
}

void enterWifiScan(void *context) {
    static_cast<AstraGlassServices *>(context)->scanWifiNow();
}

const char *speakerTest(void *context) {
    return static_cast<AstraGlassServices *>(context)->speakerTest();
}

const char *moveDisplayUp(void *context) {
    return static_cast<AstraGlassServices *>(context)->nudgeDisplay(0, -5);
}

const char *moveDisplayDown(void *context) {
    return static_cast<AstraGlassServices *>(context)->nudgeDisplay(0, 5);
}

const char *moveDisplayLeft(void *context) {
    return static_cast<AstraGlassServices *>(context)->nudgeDisplay(-5, 0);
}

const char *moveDisplayRight(void *context) {
    return static_cast<AstraGlassServices *>(context)->nudgeDisplay(5, 0);
}

const char *saveDisplay(void *context) {
    return static_cast<AstraGlassServices *>(context)->saveDisplayCalibration();
}

const char *setBrightness25(void *context) {
    return static_cast<AstraGlassServices *>(context)->setBrightnessLevel(64);
}

const char *setBrightness50(void *context) {
    return static_cast<AstraGlassServices *>(context)->setBrightnessLevel(128);
}

const char *setBrightness75(void *context) {
    return static_cast<AstraGlassServices *>(context)->setBrightnessLevel(192);
}

const char *setBrightness100(void *context) {
    return static_cast<AstraGlassServices *>(context)->setBrightnessLevel(255);
}

const char *sleepDevice(void *context) {
    return static_cast<AstraGlassServices *>(context)->sleepNow();
}

const char *toggleWifi(void *context) {
    return static_cast<AstraGlassServices *>(context)->toggleWifi();
}

const char *toggleGesture(void *context) {
    return static_cast<AstraGlassServices *>(context)->toggleGesture();
}

const char *setChinese(void *context) {
    return static_cast<AstraGlassServices *>(context)->setLanguage(
        astra_settings::Language::Chinese);
}

const char *setEnglish(void *context) {
    return static_cast<AstraGlassServices *>(context)->setLanguage(
        astra_settings::Language::English);
}

const char *startDinoJump(void *context) {
    static_cast<AstraDinoJumpController *>(context)->enter();
    return nullptr;
}

astra::List *action(const std::string &title,
                    void *context,
                    astra::Menu::ActionCallback callback) {
    auto *item = new astra::List(title);
    item->setAction(callback, context);
    return item;
}

astra::List *localizedAction(TextId id,
                             AstraGlassServices *services,
                             void *context,
                             astra::Menu::ActionCallback callback) {
    auto *item = action(astra_language::text(id, astra_settings::Language::Chinese),
                        context,
                        callback);
    localizeMenu(item, id, services);
    return item;
}

AstraPortPages buildFeaturePages(AstraGlassServices *services,
                                 AstraDinoJumpController *dinoJump) {
    AstraPortPages pages;
    const std::vector<unsigned char> deviceIcon = astra_port_icons::device();

    pages.root = localizedTile(TextId::Home, services, deviceIcon);
    pages.cameraPage = localizedList(TextId::Camera, services, astra_port_icons::camera());
    pages.audioPage = localizedList(TextId::Audio, services, astra_port_icons::audio());
    pages.radioPage = localizedList(TextId::Radio, services, astra_port_icons::radio());
    pages.loraPage = localizedList(TextId::LoRa, services, astra_port_icons::lora());
    pages.networkPage = localizedList(TextId::Network, services, astra_port_icons::network());
    pages.gamePage = localizedList(TextId::Games, services, astra_port_icons::game());
    pages.devicePage = localizedList(TextId::Device, services, deviceIcon);
    pages.secondPage = pages.devicePage;

    pages.root->addItem(pages.cameraPage);
    pages.root->addItem(pages.audioPage);
    pages.root->addItem(pages.radioPage);
    pages.root->addItem(pages.loraPage);
    pages.root->addItem(pages.networkPage);
    pages.root->addItem(pages.gamePage);
    pages.root->addItem(pages.devicePage);

    pages.cameraPage->addItem(new AstraStatusPage(TextId::CameraStatus,
                                                  services,
                                                  renderCameraStatus));
    pages.cameraPage->addItem(new AstraStatusPage(TextId::Gesture3D,
                                                  services,
                                                  renderGesture3DStatus,
                                                  enterGesture3D,
                                                  exitGesture3D));
    pages.cameraPage->addItem(new AstraStatusPage(TextId::GestureControl,
                                                  services,
                                                  renderGestureControlStatus,
                                                  enterGestureControl,
                                                  exitGestureControl));
    pages.cameraPage->addItem(new AstraStatusPage(TextId::CameraStream,
                                                  services,
                                                  renderCameraStreamStatus,
                                                  enterCameraStream,
                                                  exitCameraStream));

    pages.audioPage->addItem(new AstraStatusPage(TextId::MicrophoneVolume,
                                                 services,
                                                 renderMicrophone,
                                                 enterMicrophone,
                                                 exitMicrophone));
    pages.audioPage->addItem(localizedAction(TextId::SpeakerTest,
                                              services,
                                              services,
                                              speakerTest));
    pages.audioPage->addItem(new AstraStatusPage(TextId::InputTest, services, renderInputTest));

    pages.radioPage->setEnterCallback(enterRadio, services);
    pages.radioPage->setExitCallback(exitRadio, services);
    for (std::size_t index = 0; index < radio_demo::kStationCount; ++index) {
        auto *context = new RadioStationContext{services, index};
        auto *station = new AstraStatusPage(fitRadioStationTitle(radio_demo::kStations[index].name),
                                             context,
                                             renderRadioDetail,
                                             enterRadioStation);
        station->setClickCallback(radioDetailClick, context);
        station->setMarqueeEnabled(true);
        station->setMarqueeText(radio_demo::kStations[index].name);
        pages.radioPage->addItem(station);
    }

    pages.loraPage->addItem(new AstraStatusPage(TextId::ContinuousTransmit,
                                                services,
                                                renderLoRaTransmit,
                                                enterLoRaTransmit,
                                                exitLoRaTransmit));
    pages.loraPage->addItem(new AstraStatusPage(TextId::ReceiveMonitor,
                                                services,
                                                renderLoRaReceive,
                                                enterLoRaReceive,
                                                exitLoRaReceive));
    pages.loraPage->addItem(new AstraStatusPage(TextId::LoRaParameters,
                                                services,
                                                renderLoRaParameters));
    pages.loraPage->addItem(new AstraStatusPage(TextId::WirelessStatus,
                                                services,
                                                renderWirelessStatus));

    pages.networkPage->addItem(new AstraStatusPage(TextId::WifiStatus, services, renderWifiStatus));
    pages.networkPage->addItem(new AstraStatusPage(TextId::WifiScan,
                                                   services,
                                                   renderWifiScan,
                                                   enterWifiScan));
    pages.networkPage->addItem(new AstraStatusPage(TextId::TimeNtp, services, renderTime));

    if (dinoJump != nullptr) {
        pages.gamePage->addItem(localizedAction(TextId::DinoJump,
                                                services,
                                                dinoJump,
                                                startDinoJump));
    }
    pages.devicePage->addItem(new AstraStatusPage(TextId::Battery, services, renderBattery));

    auto *calibration = localizedList(TextId::DisplayCalibration, services);
    calibration->addItem(localizedAction(TextId::MoveUp, services, services, moveDisplayUp));
    calibration->addItem(localizedAction(TextId::MoveDown, services, services, moveDisplayDown));
    calibration->addItem(localizedAction(TextId::MoveLeft, services, services, moveDisplayLeft));
    calibration->addItem(localizedAction(TextId::MoveRight, services, services, moveDisplayRight));
    calibration->addItem(localizedAction(TextId::Save, services, services, saveDisplay));
    pages.devicePage->addItem(calibration);

    auto *brightness = localizedList(TextId::Brightness, services);
    brightness->addItem(action("25%", services, setBrightness25));
    brightness->addItem(action("50%", services, setBrightness50));
    brightness->addItem(action("75%", services, setBrightness75));
    brightness->addItem(action("100%", services, setBrightness100));
    pages.devicePage->addItem(brightness);

    auto *language = localizedList(TextId::LanguageMenu, services);
    language->addItem(localizedAction(TextId::Chinese, services, services, setChinese),
                      new astra::CheckBox(services->languageChineseSetting()));
    language->addItem(localizedAction(TextId::English, services, services, setEnglish),
                      new astra::CheckBox(services->languageEnglishSetting()));
    pages.devicePage->addItem(language);

    auto *settings = localizedList(TextId::Settings, services);
    auto *wifiSetting = localizedAction(TextId::Wifi, services, services, toggleWifi);
    settings->addItem(wifiSetting,
                      new astra::CheckBox(services->wifiEnabledSetting()));
    auto *gestureSetting = localizedAction(TextId::GestureRecognition,
                                           services,
                                           services,
                                           toggleGesture);
    settings->addItem(gestureSetting,
                      new astra::CheckBox(services->gestureEnabledSetting()));
    pages.devicePage->addItem(settings);

    pages.devicePage->addItem(localizedAction(TextId::Sleep, services, services, sleepDevice));
    pages.devicePage->addItem(new AstraStatusPage(TextId::FactoryDiagnostics,
                                                  services,
                                                  renderDiagnostics));

    return pages;
}

} // namespace

AstraPortPages buildAstraPortPages(AstraGlassServices &services) {
    return buildFeaturePages(&services, nullptr);
}

AstraPortPages buildAstraPortPages(AstraGlassServices &services,
                                   AstraDinoJumpController &dinoJump) {
    return buildFeaturePages(&services, &dinoJump);
}

AstraPortPages buildAstraPortPages(bool &test,
                                   unsigned char &testIndex,
                                   unsigned char &testSlider) {
    AstraPortPages pages;
    const std::vector<unsigned char> cameraIcon = astra_port_icons::camera();
    const std::vector<unsigned char> audioIcon = astra_port_icons::audio();
    const std::vector<unsigned char> loraIcon = astra_port_icons::lora();
    const std::vector<unsigned char> networkIcon = astra_port_icons::network();
    const std::vector<unsigned char> deviceIcon = astra_port_icons::device();

    auto *rootPage = new astra::Tile("root", deviceIcon);
    auto *secondPage = new astra::List("secondPage", deviceIcon);

    rootPage->addItem(new astra::List("test1", cameraIcon));
    rootPage->addItem(new astra::List("测试2", audioIcon));
    rootPage->addItem(new astra::List("测试测试3", loraIcon));
    rootPage->addItem(new astra::List("测试测试3", networkIcon));
    rootPage->addItem(secondPage);

    secondPage->addItem(new astra::List());
    secondPage->addItem(new astra::List("-测试2"), new astra::CheckBox(test));
    secondPage->addItem(new astra::Tile("-测试测试3"),
                        new astra::PopUp(1, "测试", {"测试"}, testIndex));
    secondPage->addItem(new astra::Tile("-测试测试测试4"),
                        new astra::Slider("测试", 0, 100, 50, testSlider));
    secondPage->addItem(new astra::List("-测试测试测试5"));
    secondPage->addItem(new astra::List("-测试测试测试6"));
    secondPage->addItem(new astra::List("-测试测试测试6"));
    secondPage->addItem(new astra::List("-测试测试测试6"));
    secondPage->addItem(new astra::List("-测试测试测试6"));

    pages.root = rootPage;
    pages.secondPage = secondPage;
    return pages;
}
