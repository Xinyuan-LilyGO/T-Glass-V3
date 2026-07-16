/**
 * @file      GlassWindown.ino
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2024  ShenZhen XinYuan Electronic Technology Co., Ltd
 * @date      2024-02-22
 *
 */
#include <LilyGo_GlassV3.h>
#include <LV_Helper.h>
#include <AudioFileSourcePROGMEM.h>
#include <AudioGeneratorMP3.h>
#include <AudioFileSourceID3.h>
#include "mp3_buffer.h"

// The resolution of the non-magnified side of the glasses reflection area is about 126x126,
// and the magnified area is smaller than 126x126
#define GlassViewableWidth              126
#define GlassViewableHeight             126

AudioFileSourcePROGMEM *file = nullptr;
AudioGeneratorMP3       *mp3 = nullptr;
AudioFileSourceID3      *id3 = nullptr;


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
lv_obj_t *label;
uint8_t volume = 10;

void setup()
{
    bool rslt = false;

    // Turn on debugging message output, Arduino IDE users please put
    // Tools -> USB CDC On Boot -> Enable, otherwise there will be no output
    Serial.begin(115200);


    // Initialization screen and peripherals
    rslt = glass.begin();
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
    // Set window border width zero
    lv_obj_set_style_border_width(window, 0, 0);
    // Set display window size
    lv_obj_set_size(window, GlassViewableWidth, GlassViewableHeight);
    // Set window position
    lv_obj_align(window, LV_ALIGN_BOTTOM_MID, 0, 0);


    label = lv_label_create(window);        /*Add a label the current screen*/
    lv_label_set_text(label, "Volume:5");          /*Set label text*/
    lv_obj_center(label);                             /*Set center alignment*/

    file = new AudioFileSourcePROGMEM();
    id3 = new AudioFileSourceID3(file);
    mp3 = new AudioGeneratorMP3();

    file->open(mp3_buffer, mp3_buffer_len);
    mp3->begin(id3, glass.audioOut);

    touchButton.begin();

}

extern AudioBoard audioOutputDev;

void loop()
{
    touchButton.update();

    if (touchButton.isShortPress()) {
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        volume += 5;
        volume %= 100;
        lv_label_set_text_fmt(label, "Volume: %d", volume);
        audioOutputDev.setVolume(volume);
    }

    if (mp3->isRunning()) {
        if (!mp3->loop()) {
            mp3->stop();
            delay(2000);
            file->open(mp3_buffer, mp3_buffer_len);
            mp3->begin(id3, glass.audioOut);
        }
    }

    // lvgl task processing should be placed in the loop function
    lv_timer_handler();
    delay(2);
}






