#pragma once

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>

namespace astra_status_bar {

inline constexpr std::int16_t kHeight = 10;
inline constexpr std::int16_t kContentTopInset = kHeight;
inline constexpr std::int16_t kWifiIconWidth = 10;
inline constexpr std::int16_t kWifiIconHeight = 10;

enum class NetworkState : std::uint8_t {
    Disabled,
    Connecting,
    Connected,
};

inline int clampBattery(int percent) {
    return std::max(0, std::min(percent, 100));
}

inline const char *networkLabel(NetworkState state) {
    switch (state) {
        case NetworkState::Connected:
            return "WiFi+";
        case NetworkState::Connecting:
            return "WiFi~";
        case NetworkState::Disabled:
        default:
            return "WiFi-";
    }
}

inline std::uint16_t wifiIconRow(NetworkState state, int row) {
    if (row < 0 || row >= kWifiIconHeight) {
        return 0;
    }

    // 10x10 alpha masks extracted from WIFI (3).png and nowifi (1).png.
    static constexpr std::uint16_t kWifi[] = {
        0x000, 0x078, 0x1FE, 0x303, 0x078, 0x0EC, 0x000, 0x030, 0x000, 0x000,
    };
    static constexpr std::uint16_t kNoWifi[] = {
        0x000, 0x0C0, 0x0FC, 0x3FF, 0x3FF, 0x1FE, 0x078, 0x07C, 0x030, 0x000,
    };

    return state == NetworkState::Connected ? kWifi[row] : kNoWifi[row];
}

inline std::string formatClock(int hour, int minute) {
    char value[6] = {};
    std::snprintf(value,
                  sizeof(value),
                  "%02d:%02d",
                  std::max(0, std::min(hour, 23)),
                  std::max(0, std::min(minute, 59)));
    return value;
}

} // namespace astra_status_bar
