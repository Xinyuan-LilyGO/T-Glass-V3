#pragma once

#include "camera/Gesture3D/AstraGesturePolicy.h"

namespace astra_gesture_control {

enum class Action : unsigned char {
    None,
    Previous,
    Next,
    Open,
    Close,
    TogglePlayback,
};

inline Action actionFor(astra_gesture::Digit digit) {
    switch (digit) {
        case astra_gesture::Digit::One:
            return Action::Previous;
        case astra_gesture::Digit::Two:
            return Action::Next;
        case astra_gesture::Digit::Three:
            return Action::Open;
        case astra_gesture::Digit::Four:
            return Action::Close;
        case astra_gesture::Digit::Five:
            return Action::TogglePlayback;
        case astra_gesture::Digit::None:
        default:
            return Action::None;
    }
}

inline bool shouldCloseLeafPage(Action action, bool hasChildren) {
    if (hasChildren) {
        return false;
    }

    switch (action) {
        case Action::Previous:
        case Action::Next:
        case Action::Open:
            return true;
        case Action::None:
        case Action::Close:
        case Action::TogglePlayback:
        default:
            return false;
    }
}

} // namespace astra_gesture_control
