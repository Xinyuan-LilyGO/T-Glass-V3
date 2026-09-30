#pragma once

#include <cstdint>

namespace astra_settings {

enum class Language : std::uint8_t {
    Chinese = 0,
    English = 1,
};

struct State {
    bool wifiEnabled;
    bool gestureEnabled;
    Language language;
};

inline constexpr char kPreferencesNamespace[] = "astra_ui";
inline constexpr char kWifiEnabledKey[] = "wifi_enabled";
inline constexpr char kGestureEnabledKey[] = "gesture_enabled";
inline constexpr char kLanguageKey[] = "language";

inline constexpr Language defaultLanguage() {
    return Language::Chinese;
}

inline constexpr State defaultState() {
    return {true, false, defaultLanguage()};
}

} // namespace astra_settings
