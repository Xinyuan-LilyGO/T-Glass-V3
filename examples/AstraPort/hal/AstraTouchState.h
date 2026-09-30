#pragma once

class AstraTouchState {
public:
    void update(bool pressed) {
        pressed_ = pressed;
    }

    bool isPressed() const {
        return pressed_;
    }

private:
    volatile bool pressed_ = false;
};
