/**
 * @file      GlassDeepSleep.ino
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2024  ShenZhen XinYuan Electronic Technology Co., Ltd
 * @date      2024-02-23
 * @note
 */
#include <LilyGo_GlassV3.h>
#include <LV_Helper.h>

// The resolution of the non-magnified side of the glasses reflection area is about 126x126,
// and the magnified area is smaller than 126x126
#define GlassViewableWidth              126
#define GlassViewableHeight             126

lv_obj_t *btn_state;
bool touchDetected;

void button_event_callback(ButtonState state)
{
    switch (state) {
    case BTN_CLICK_EVENT:
        Serial.println("Click");
        lv_label_set_text(btn_state, "Click");
        lv_obj_align(btn_state, LV_ALIGN_CENTER, 0, 0);
        break;
    case BTN_LONG_PRESSED_EVENT:

        Serial.println("Long Pressed");

        lv_label_set_text(btn_state, "Sleep Start");
        lv_obj_align(btn_state, LV_ALIGN_CENTER, 0, 0);

        // Set touch button wake-up and set the touch threshold for wake-up
        glass.enableTouchWakeup(200);

        Serial.println("Sleep Start!");

        // Display label
        lv_timer_handler();

        // Wait for a while
        delay(5000);

        // Go to sleep
        glass.sleep();

        Serial.println("This place will never print!");

        break;
    default:
        break;
    }

}


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
    btn_state = lv_label_create(window);
    lv_obj_set_style_text_color(btn_state, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(btn_state, &lv_font_montserrat_22, LV_PART_MAIN);
    lv_label_set_text(btn_state, "Wakeup");
    lv_obj_align(btn_state, LV_ALIGN_CENTER, 0, 0);

}


void loop()
{
    // Check button state
    glass.update();
    // lvgl task processing should be placed in the loop function
    lv_timer_handler();
    delay(5);
}
