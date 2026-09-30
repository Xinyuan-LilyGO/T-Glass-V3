#pragma once

#include <cstdint>
#include <cstring>

namespace astra_gesture {

enum class Action : std::uint8_t {
    None,
    Previous,
    Next,
    Open,
    Close,
};

constexpr float kMinimumScore = 0.80F;

inline Action actionFor(const char *category, float score) {
    if (category == nullptr || score < kMinimumScore) {
        return Action::None;
    }
    if (std::strcmp(category, "one") == 0) {
        return Action::Previous;
    }
    if (std::strcmp(category, "two") == 0) {
        return Action::Next;
    }
    if (std::strcmp(category, "ok") == 0) {
        return Action::Open;
    }
    if (std::strcmp(category, "dislike") == 0) {
        return Action::Close;
    }
    return Action::None;
}

class Debouncer {
public:
    explicit Debouncer(std::uint32_t cooldownMs) : cooldownMs_(cooldownMs) {
    }

    bool accept(Action action, std::uint32_t nowMs) {
        if (action == Action::None) {
            return false;
        }
        if (action != lastAction_ || static_cast<std::uint32_t>(nowMs - lastTimeMs_) >= cooldownMs_) {
            lastAction_ = action;
            lastTimeMs_ = nowMs;
            return true;
        }
        return false;
    }

private:
    const std::uint32_t cooldownMs_;
    Action lastAction_ = Action::None;
    std::uint32_t lastTimeMs_ = 0;
};

} // namespace astra_gesture
