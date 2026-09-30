#pragma once

#include <cstddef>
#include <cstdint>

namespace radio_demo {

struct RadioDemoStation {
    const char *name;
    const char *url;
};

static const char kWifiSsid[] = "YOUR_WIFI_SSID";
static const char kWifiPassword[] = "YOUR_WIFI_PASSWORD";

static const RadioDemoStation kStations[] = {
    {"上海动感101", "http://lhttp.qingting.fm/live/274/64k.mp3"},
    {"Qingting FM 5022", "http://lhttp.qingting.fm/live/5022/64k.mp3"},
    {"北京文艺广播", "http://lhttp.qingting.fm/live/333/64k.mp3"},
    {"怀集音乐之声", "http://lhttp.qingting.fm/live/4804/64k.mp3"},
    {"两广之声音乐台", "http://lhttp.qingting.fm/live/20500149/64k.mp3"},
    {"清晨音乐台", "http://lhttp.qingting.fm/live/4915/64k.mp3"},
    {"河北音乐广播", "http://lhttp.qingting.fm/live/1649/64k.mp3"},
    {"上海流行音乐LoveRadio", "http://lhttp.qingting.fm/live/273/64k.mp3"},
    {"江苏经典流行音乐", "http://lhttp.qingting.fm/live/4938/64k.mp3"},
    {"广东音乐之声", "http://lhttp.qingting.fm/live/1260/64k.mp3"},
    {"怀旧好声音", "http://lhttp.qingting.fm/live/1223/64k.mp3"},
    {"哈尔滨音乐广播", "http://lhttp.qingting.fm/live/839/64k.mp3"},
    {"苏州音乐广播", "http://lhttp.qingting.fm/live/2803/64k.mp3"},
    {"959年代音乐怀旧好声音", "http://lhttp.qingting.fm/live/5021381/64k.mp3"},
    {"动听音乐台", "http://lhttp.qingting.fm/live/5022107/64k.mp3"},
    {"500首华语经典", "http://lhttp.qingting.fm/live/5022308/64k.mp3"},
    {"北京音乐广播", "http://lhttp.qingting.fm/live/332/64k.mp3"},
    {"954汽车音乐广播", "http://lhttp.qingting.fm/live/1936/64k.mp3"},
    {"深圳飞扬971", "http://lhttp.qingting.fm/live/1271/64k.mp3"},
    {"950广西音乐台", "http://lhttp.qingting.fm/live/4875/64k.mp3"},
    {"厦门音乐广播", "http://lhttp.qingting.fm/live/1739/64k.mp3"},
    {"欧美音乐88.7", "http://lhttp.qingting.fm/live/15318703/64k.mp3"},
};

static constexpr std::size_t kStationCount = sizeof(kStations) / sizeof(kStations[0]);
static constexpr std::size_t kStreamBufferBytes = 64 * 1024;
static constexpr std::size_t kMp3PreallocateBytes = 32 * 1024;
static constexpr std::size_t kPrebufferBytes = 24 * 1024;
static constexpr std::size_t kMinimumStartBufferBytes = 8 * 1024;
static constexpr std::uint32_t kPrebufferTimeoutMs = 6000;
static constexpr int kStreamReconnectTries = 3;
static constexpr int kStreamReconnectDelayMs = 250;
static constexpr std::uint32_t kRetryDelayMs = 3000;
static constexpr std::uint8_t kDefaultVolume = 50;

}  // namespace radio_demo
