#pragma once

#include <cstdint>
#include <cstring>

namespace astra_gesture {

enum class Digit : std::uint8_t {
    None,
    One,
    Two,
    Three,
    Four,
    Five,
};

constexpr float kMinimumScore = 0.80F;

inline Digit digitFor(const char *category, float score) {
    if (category == nullptr || score < kMinimumScore) {
        return Digit::None;
    }
    if (std::strcmp(category, "one") == 0) {
        return Digit::One;
    }
    if (std::strcmp(category, "two") == 0) {
        return Digit::Two;
    }
    if (std::strcmp(category, "three") == 0) {
        return Digit::Three;
    }
    if (std::strcmp(category, "four") == 0) {
        return Digit::Four;
    }
    if (std::strcmp(category, "five") == 0) {
        return Digit::Five;
    }
    return Digit::None;
}

class Debouncer {
public:
    explicit Debouncer(std::uint32_t cooldownMs) : cooldownMs_(cooldownMs) {
    }

    void reset() {
        lastDigit_ = Digit::None;
        lastTimeMs_ = 0;
    }

    bool accept(Digit digit, std::uint32_t nowMs) {
        if (digit == Digit::None) {
            return false;
        }
        if (digit != lastDigit_ || static_cast<std::uint32_t>(nowMs - lastTimeMs_) >= cooldownMs_) {
            lastDigit_ = digit;
            lastTimeMs_ = nowMs;
            return true;
        }
        return false;
    }

private:
    const std::uint32_t cooldownMs_;
    Digit lastDigit_ = Digit::None;
    std::uint32_t lastTimeMs_ = 0;
};

} // namespace astra_gesture
