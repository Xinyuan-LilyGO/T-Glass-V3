#include "RadioDemoPlayer.h"

#include <Arduino.h>
#include <WiFi.h>

#include <AudioFileSourceBuffer.h>
#include <AudioFileSourceICYStream.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutput.h>

#include <esp_heap_caps.h>

#include <cstring>

namespace radio_demo {

namespace {

void copyText(char *destination, std::size_t destinationSize, const char *source) {
    if (destination == nullptr || destinationSize == 0) return;
    if (source == nullptr) {
        destination[0] = '\0';
        return;
    }
    std::strncpy(destination, source, destinationSize - 1);
    destination[destinationSize - 1] = '\0';
}

}  // namespace

void RadioDemoPlayer::begin(AudioOutput *output,
                            const RadioDemoStation *stations,
                            std::size_t stationCount) {
    stopStream();
    releasePsramBuffers();

    output_ = output;
    stations_ = stations;
    stationCount_ = stationCount;
    controls_.reset(stationCount_, kDefaultVolume);
    retryAt_ = 0;
    lastStatusCode_ = 0;
    streamTitle_[0] = '\0';
    lastStatus_[0] = '\0';
    applyVolume();

    if (output_ == nullptr || stations_ == nullptr || stationCount_ == 0 ||
        !allocatePsramBuffers()) {
        setState(RadioDemoState::Error);
    } else if (WiFi.status() == WL_CONNECTED) {
        setState(RadioDemoState::Connecting);
    } else {
        setState(RadioDemoState::WaitingForWifi);
    }
}

void RadioDemoPlayer::update() {
    if (output_ == nullptr || stations_ == nullptr || stationCount_ == 0) {
        stopStream();
        setState(RadioDemoState::Error);
        return;
    }

    if (WiFi.status() != WL_CONNECTED) {
        stopStream();
        setState(RadioDemoState::WaitingForWifi);
        return;
    }

    if (!controls_.wantsPlayback()) {
        stopStream();
        setState(RadioDemoState::Paused);
        return;
    }

    if (decoder_ != nullptr && decoder_->isRunning()) {
        if (!decoder_->loop()) {
            scheduleRetry();
        } else {
            setState(RadioDemoState::Playing);
        }
        return;
    }

    if (buffer_ != nullptr && state_ == RadioDemoState::Buffering) {
        continueBuffering();
        return;
    }

    if (static_cast<std::int32_t>(millis() - retryAt_) >= 0) {
        startStream();
    } else {
        setState(RadioDemoState::Retrying);
    }
}

void RadioDemoPlayer::selectNextStation() {
    if (stationCount_ == 0) {
        controls_.selectNextStation();
        setState(RadioDemoState::Error);
        return;
    }
    selectStation((controls_.stationIndex() + 1) % stationCount_);
}

bool RadioDemoPlayer::selectStation(std::size_t index) {
    if (!controls_.selectStation(index)) {
        setState(RadioDemoState::Error);
        return false;
    }

    streamTitle_[0] = '\0';
    retryAt_ = 0;
    stopStream();

    if (output_ == nullptr || stations_ == nullptr || stationCount_ == 0) {
        setState(RadioDemoState::Error);
    } else if (WiFi.status() == WL_CONNECTED) {
        setState(RadioDemoState::Connecting);
    } else {
        setState(RadioDemoState::WaitingForWifi);
    }
    return true;
}

void RadioDemoPlayer::togglePlayback() {
    const bool wantsPlayback = controls_.togglePlayback();
    retryAt_ = 0;
    stopStream();

    if (!wantsPlayback) {
        setState(RadioDemoState::Paused);
    } else if (WiFi.status() == WL_CONNECTED) {
        setState(RadioDemoState::Connecting);
    } else {
        setState(RadioDemoState::WaitingForWifi);
    }
}

void RadioDemoPlayer::setPlaybackEnabled(bool enabled) {
    controls_.setPlaybackEnabled(enabled);
    retryAt_ = 0;
    stopStream();

    if (!enabled) {
        setState(RadioDemoState::Paused);
    } else if (WiFi.status() == WL_CONNECTED) {
        setState(RadioDemoState::Connecting);
    } else {
        setState(RadioDemoState::WaitingForWifi);
    }
}

void RadioDemoPlayer::increaseVolume(std::uint8_t step) {
    controls_.increaseVolume(step);
    applyVolume();
}

RadioDemoState RadioDemoPlayer::state() const {
    return state_;
}

std::size_t RadioDemoPlayer::stationIndex() const {
    return controls_.stationIndex();
}

const RadioDemoStation *RadioDemoPlayer::station() const {
    if (stations_ == nullptr || stationCount_ == 0) return nullptr;
    return &stations_[controls_.stationIndex()];
}

const char *RadioDemoPlayer::streamTitle() const {
    return streamTitle_;
}

const char *RadioDemoPlayer::statusMessage() const {
    return lastStatus_;
}

std::uint8_t RadioDemoPlayer::volume() const {
    return controls_.volume();
}

bool RadioDemoPlayer::playbackEnabled() const {
    return controls_.wantsPlayback();
}

void RadioDemoPlayer::startStream() {
    stopStream();
    setState(RadioDemoState::Connecting);
    streamTitle_[0] = '\0';
    lastStatusCode_ = 0;
    lastStatus_[0] = '\0';

    const RadioDemoStation *selected = station();
    if (selected == nullptr || output_ == nullptr) {
        scheduleRetry();
        setState(RadioDemoState::Error);
        return;
    }

    if (streamBuffer_ == nullptr || codecMemory_ == nullptr) {
        copyText(lastStatus_, sizeof(lastStatus_), "PSRAM audio buffers unavailable");
        scheduleRetry();
        return;
    }

    stream_ = new AudioFileSourceICYStream();
    if (stream_ == nullptr) {
        scheduleRetry();
        return;
    }
    stream_->useHTTP10();
    stream_->SetReconnect(kStreamReconnectTries, kStreamReconnectDelayMs);
    if (!stream_->open(selected->url) || !stream_->isOpen()) {
        scheduleRetry();
        return;
    }
    stream_->RegisterMetadataCB(metadataCallback, this);

    buffer_ = new AudioFileSourceBuffer(stream_, streamBuffer_, kStreamBufferBytes);
    if (buffer_ == nullptr) {
        scheduleRetry();
        return;
    }
    buffer_->RegisterStatusCB(statusCallback, this);

    bufferPrimed_ = false;
    bufferingStartedAt_ = millis();
    setState(RadioDemoState::Buffering);
}

void RadioDemoPlayer::continueBuffering() {
    if (stream_ == nullptr || buffer_ == nullptr || !stream_->isOpen()) {
        scheduleRetry();
        return;
    }

    if (!bufferPrimed_) {
        std::uint8_t marker = 0;
        buffer_->read(&marker, 0);
        bufferPrimed_ = true;
    }

    if (!buffer_->loop()) {
        scheduleRetry();
        return;
    }

    const std::size_t fillLevel = buffer_->getFillLevel();
    const bool targetReached = fillLevel >= kPrebufferBytes;
    const bool timedOut = static_cast<std::uint32_t>(millis() - bufferingStartedAt_) >=
                          kPrebufferTimeoutMs;
    if (!targetReached && !timedOut) {
        setState(RadioDemoState::Buffering);
        return;
    }

    if (fillLevel < kMinimumStartBufferBytes) {
        copyText(lastStatus_, sizeof(lastStatus_), "Radio prebuffer timeout");
        scheduleRetry();
        return;
    }

    startDecoder();
}

void RadioDemoPlayer::startDecoder() {
    decoder_ = new AudioGeneratorMP3(codecMemory_, kMp3PreallocateBytes);
    if (decoder_ == nullptr) {
        scheduleRetry();
        return;
    }
    decoder_->RegisterStatusCB(statusCallback, this);
    if (!decoder_->begin(buffer_, output_)) {
        scheduleRetry();
        return;
    }

    applyVolume();
    setState(RadioDemoState::Playing);
}

void RadioDemoPlayer::stopStream() {
    if (decoder_ != nullptr) {
        decoder_->stop();
        delete decoder_;
        decoder_ = nullptr;
    }
    if (buffer_ != nullptr) {
        buffer_->close();
        delete buffer_;
        buffer_ = nullptr;
    }
    if (stream_ != nullptr) {
        stream_->close();
        delete stream_;
        stream_ = nullptr;
    }
    bufferPrimed_ = false;
    bufferingStartedAt_ = 0;
    streamTitle_[0] = '\0';
}

bool RadioDemoPlayer::allocatePsramBuffers() {
    streamBuffer_ = static_cast<std::uint8_t *>(
        heap_caps_malloc(kStreamBufferBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    codecMemory_ = heap_caps_malloc(kMp3PreallocateBytes,
                                    MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (streamBuffer_ != nullptr && codecMemory_ != nullptr) return true;

    copyText(lastStatus_, sizeof(lastStatus_), "PSRAM audio allocation failed");
    releasePsramBuffers();
    return false;
}

void RadioDemoPlayer::releasePsramBuffers() {
    if (codecMemory_ != nullptr) {
        heap_caps_free(codecMemory_);
        codecMemory_ = nullptr;
    }
    if (streamBuffer_ != nullptr) {
        heap_caps_free(streamBuffer_);
        streamBuffer_ = nullptr;
    }
}

void RadioDemoPlayer::scheduleRetry() {
    stopStream();
    retryAt_ = millis() + kRetryDelayMs;
    setState(RadioDemoState::Retrying);
}

void RadioDemoPlayer::applyVolume() {
    if (output_ != nullptr) {
        output_->SetGain(static_cast<float>(controls_.volume()) / 100.0f);
    }
}

void RadioDemoPlayer::setState(RadioDemoState state) {
    state_ = state;
}

void RadioDemoPlayer::metadataCallback(void *context,
                                       const char *type,
                                       bool isUnicode,
                                       const char *value) {
    (void)isUnicode;
    if (context == nullptr || type == nullptr || value == nullptr) return;
    if (std::strstr(type, "Title") == nullptr) return;

    auto *player = static_cast<RadioDemoPlayer *>(context);
    copyText(player->streamTitle_, sizeof(player->streamTitle_), value);
}

void RadioDemoPlayer::statusCallback(void *context, int code, const char *message) {
    if (context == nullptr) return;

    auto *player = static_cast<RadioDemoPlayer *>(context);
    player->lastStatusCode_ = code;
    copyText(player->lastStatus_, sizeof(player->lastStatus_), message);
}

}  // namespace radio_demo
