#pragma once

#include <cstddef>
#include <cstdint>

#include "RadioDemoConfig.h"

class AudioOutput;
class AudioFileSourceICYStream;
class AudioFileSourceBuffer;
class AudioGeneratorMP3;

namespace radio_demo {

enum class RadioDemoState : std::uint8_t {
    Idle,
    WaitingForWifi,
    Connecting,
    Buffering,
    Playing,
    Paused,
    Retrying,
    Error,
};

class RadioDemoControlState {
public:
    void reset(std::size_t stationCount, std::uint8_t initialVolume) {
        stationCount_ = stationCount;
        stationIndex_ = 0;
        volume_ = initialVolume > 100 ? 100 : initialVolume;
        if (volume_ == 0) volume_ = 10;
        wantsPlayback_ = stationCount_ != 0;
    }

    std::size_t selectNextStation() {
        if (stationCount_ == 0) {
            wantsPlayback_ = false;
            return stationIndex_;
        }
        selectStation((stationIndex_ + 1) % stationCount_);
        return stationIndex_;
    }

    bool selectStation(std::size_t index) {
        if (stationCount_ == 0 || index >= stationCount_) {
            wantsPlayback_ = false;
            return false;
        }
        stationIndex_ = index;
        wantsPlayback_ = true;
        return true;
    }

    bool togglePlayback() {
        if (stationCount_ == 0) {
            wantsPlayback_ = false;
            return false;
        }
        wantsPlayback_ = !wantsPlayback_;
        return wantsPlayback_;
    }

    void setPlaybackEnabled(bool enabled) {
        wantsPlayback_ = enabled && stationCount_ != 0;
    }

    std::uint8_t increaseVolume(std::uint8_t step) {
        if (step == 0) return volume_;
        const unsigned int next = static_cast<unsigned int>(volume_) + step;
        volume_ = next >= 100 ? 10 : static_cast<std::uint8_t>(next);
        return volume_;
    }

    std::size_t stationIndex() const { return stationIndex_; }
    std::uint8_t volume() const { return volume_; }
    bool wantsPlayback() const { return wantsPlayback_; }

private:
    std::size_t stationCount_ = 0;
    std::size_t stationIndex_ = 0;
    std::uint8_t volume_ = 10;
    bool wantsPlayback_ = false;
};

class RadioDemoPlayer {
public:
    void begin(AudioOutput *output,
               const RadioDemoStation *stations,
               std::size_t stationCount);
    void update();
    bool selectStation(std::size_t index);
    void selectNextStation();
    void togglePlayback();
    void setPlaybackEnabled(bool enabled);
    void increaseVolume(std::uint8_t step = 10);

    RadioDemoState state() const;
    std::size_t stationIndex() const;
    const RadioDemoStation *station() const;
    const char *streamTitle() const;
    const char *statusMessage() const;
    std::uint8_t volume() const;
    bool playbackEnabled() const;

private:
    static void metadataCallback(void *context,
                                 const char *type,
                                 bool isUnicode,
                                 const char *value);
    static void statusCallback(void *context, int code, const char *message);

    void startStream();
    void continueBuffering();
    void startDecoder();
    void stopStream();
    bool allocatePsramBuffers();
    void releasePsramBuffers();
    void scheduleRetry();
    void applyVolume();
    void setState(RadioDemoState state);

    AudioOutput *output_ = nullptr;
    const RadioDemoStation *stations_ = nullptr;
    std::size_t stationCount_ = 0;
    RadioDemoControlState controls_;
    RadioDemoState state_ = RadioDemoState::Idle;
    AudioFileSourceICYStream *stream_ = nullptr;
    AudioFileSourceBuffer *buffer_ = nullptr;
    AudioGeneratorMP3 *decoder_ = nullptr;
    std::uint8_t *streamBuffer_ = nullptr;
    void *codecMemory_ = nullptr;
    bool bufferPrimed_ = false;
    std::uint32_t bufferingStartedAt_ = 0;
    std::uint32_t retryAt_ = 0;
    int lastStatusCode_ = 0;
    char streamTitle_[128] = {};
    char lastStatus_[96] = {};
};

}  // namespace radio_demo
