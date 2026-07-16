/**
 * @file      GlassTouchButton.ino
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2024  ShenZhen XinYuan Electronic Technology Co., Ltd
 * @date      2024-02-22
 */
#include <LilyGo_GlassV3.h>
#include <LV_Helper.h>

// The resolution of the non-magnified side of the glasses reflection area is about 126x126,
// and the magnified area is smaller than 126x126
#define GlassViewableWidth              126
#define GlassViewableHeight             126


lv_obj_t *btn_value;
uint32_t interval;

class TouchButton
{
public:
    TouchButton(int pin, unsigned long longPressTime = 1000, bool activeHigh = true)
    {
        _pin = pin;
        _longPressTime = longPressTime;
        _activeHigh = activeHigh;
        _lastState = !activeHigh;
        _pressStartTime = 0;
        _shortPressFlag = false;
        _longPressFlag = false;
        _releaseFlag = false;
        _pressed = false;
        _longPressTriggered = false;
    }

    void begin()
    {
        pinMode(_pin, INPUT);
    }

    void update()
    {
        int currentState = digitalRead(_pin);
        if (!_activeHigh) {
            currentState = (currentState == HIGH) ? LOW : HIGH;
        }

        if (currentState == HIGH && _lastState == LOW) {
            _pressStartTime = millis();
            _pressed = true;
            _longPressTriggered = false;
        } else if (currentState == HIGH && _lastState == HIGH) {
            if (_pressed && !_longPressTriggered) {
                if (millis() - _pressStartTime >= _longPressTime) {
                    _longPressFlag = true;
                    _longPressTriggered = true;
                }
            }
        } else if (currentState == LOW && _lastState == HIGH) {
            if (_pressed) {
                _releaseFlag = true;
                if (!_longPressTriggered) {
                    _shortPressFlag = true;
                }
            }
            _pressed = false;
        }

        _lastState = currentState;
    }

    bool isShortPress()
    {
        if (_shortPressFlag) {
            _shortPressFlag = false;
            return true;
        }
        return false;
    }

    bool isLongPress()
    {
        if (_longPressFlag) {
            _longPressFlag = false;
            return true;
        }
        return false;
    }

    bool isRelease()
    {
        if (_releaseFlag) {
            _releaseFlag = false;
            return true;
        }
        return false;
    }

    bool isPressed()
    {
        return _pressed;
    }

private:
    int _pin;
    unsigned long _longPressTime;
    bool _activeHigh;
    int _lastState;
    unsigned long _pressStartTime;
    bool _shortPressFlag;
    bool _longPressFlag;
    bool _releaseFlag;
    bool _pressed;
    bool _longPressTriggered;
};


TouchButton touchButton(BOARD_TOUCH_BUTTON);

void setup()
{
    // Turn on debugging message output, Arduino IDE users please put
    // Tools -> USB CDC On Boot -> Enable, otherwise there will be no output
    Serial.begin(115200);

    // Initialization screen and peripherals
    bool rslt = glass.begin();
    if (!rslt) {
        while (1) {
            Serial.println("The board model cannot be detected, please raise the Core Debug Level to an error");
            delay(1000);
        }
    }

    // Brightness range : 0 ~ 255
    glass.setBrightness(255);

    // Initialize lvgl
    beginLvglHelper(glass);

    // Set display background color to black
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), 0);

    // Create a display window object
    lv_obj_t *window = lv_obj_create(lv_scr_act());
    // Set window background color to black
    lv_obj_set_style_bg_color(window, lv_color_black(), 0);
    // Set window border width zero
    lv_obj_set_style_border_width(window, 0, 0);
    // Set display window size
    lv_obj_set_size(window, GlassViewableWidth, GlassViewableHeight);
    // Set window position
    lv_obj_align(window, LV_ALIGN_BOTTOM_MID, 0, 0);

    // Create button state label
    btn_value = lv_label_create(window);
    lv_obj_set_style_text_color(btn_value, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(btn_value, &lv_font_montserrat_22, LV_PART_MAIN);
    lv_label_set_text(btn_value, "Release");
    lv_obj_align(btn_value, LV_ALIGN_CENTER, 0, 0);

    touchButton.begin();
}


void loop()
{
    touchButton.update();

    if (touchButton.isShortPress()) {
        Serial.println("Short Press Detected");
        lv_label_set_text_fmt(btn_value, "Short Press");
        lv_obj_align(btn_value, LV_ALIGN_CENTER, 0, 0);
    }
    if (touchButton.isLongPress()) {
        Serial.println("Long Press Detected");
        lv_label_set_text_fmt(btn_value, "Long Press");
        lv_obj_align(btn_value, LV_ALIGN_CENTER, 0, 0);
    }
    if (touchButton.isRelease()) {
        Serial.println("Release Detected");
    }

    lv_timer_handler();
    delay(1);
}
