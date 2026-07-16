/**
 * @file      GlassFactory.ino
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2024  ShenZhen XinYuan Electronic Technology Co., Ltd
 * @date      2023-10-23
 * @note      Arduino Setting
 *            Tools ->
 *              Board:"ESP32S3 Dev Module"
 *              USB CDC On Boot:"Enable"
 *              USB DFU On Boot:"Disable"
 *              Flash Size : "4MB(32Mb)"
 *              Flash Mode"QIO 80MHz
 *              Partition Scheme:"Huge APP (3MB No OTA/1MB SPIFFS)"
 *              PSRAM:"QSPI PSRAM"
 *              Upload Mode:"UART0/Hardware CDC"
 *              USB Mode:"Hardware CDC and JTAG"
 */
#include <LV_Helper.h>
#include <LilyGo_GlassV3.h>
#include <WiFi.h>
#include <esp_sntp.h>
#include <esp_camera.h>
#include <Preferences.h>

#ifndef RADIO_FREQ
#ifdef  JAPAN_MIC
#define RADIO_FREQ           920.0
#else
#define RADIO_FREQ           868.0
#endif
#endif

#ifndef RADIO_BANDWIDTH
#define RADIO_BANDWIDTH      125.0
#endif

#ifndef RADIO_SF
#define RADIO_SF             10
#endif

#ifndef RADIO_CR
#define RADIO_CR             6
#endif

#ifndef RADIO_TX_POWER
#define RADIO_TX_POWER       22
#endif

extern SX1262 radio;
volatile bool transmissionFlag = false;

// The resolution of the non-magnified side of the glasses reflection area is about 126x126,
// and the magnified area is smaller than 126x126
#define GlassViewableWidth              126
#define GlassViewableHeight             126

LV_IMG_DECLARE(img_down);
LV_IMG_DECLARE(img_left);
LV_IMG_DECLARE(img_right);
LV_IMG_DECLARE(img_up);

struct GlassSetting {
    float lora_freq;
    float lora_bw;
    uint8_t lora_cr;
    uint8_t lora_tx_power;
    uint8_t lora_sf;
    uint8_t lora_sw;
    uint8_t lora_preamble_length;
    bool lora_tx_enable;
    bool touch_enter_sleep;
    uint32_t touch_threshold;
};

static struct GlassSetting glass_setting = {
    .lora_freq = RADIO_FREQ,
    .lora_bw = RADIO_BANDWIDTH,
    .lora_cr = RADIO_CR,
    .lora_tx_power = RADIO_TX_POWER,
    .lora_sf = RADIO_SF,
    .lora_sw = 0x12,
    .lora_preamble_length = 15,
    .lora_tx_enable = false,
    .touch_enter_sleep = false,
    .touch_threshold = 200,
};

Preferences preferences;
enum FactoryUIPageID {
    PAGE_CAMERA,
    PAGE_MIC_LEVEL,
    PAGE_DATETIME,
    PAGE_LORA_TX,
    PAGE_LORA_RX,
    PAGE_BATTERY_VOLTAGE,
    PAGE_WIFI,
    PAGE_IMAGE,
    PAGE_NOISE_DETECT
};

static lv_color_t font_color = lv_color_white();
static lv_color_t bg_color = lv_color_black();

static lv_obj_t *touch_label;

static lv_obj_t *tileview;
static lv_obj_t *time_label;
static lv_obj_t *day_label;
static lv_obj_t *week_label;
static lv_obj_t *month_label;
static bool colon;
static lv_obj_t *wifi_label_rssi;
static lv_obj_t *wifi_rssi_meter;
static lv_meter_indicator_t *wifi_rssi_indic2 ;
static lv_obj_t *audio_level_label;
static lv_obj_t *audio_level_left_bar;
static lv_obj_t *audio_level_right_bar;
static lv_obj_t *lora_tx_message_label;
static lv_obj_t *lora_rx_message_label;
static bool shutdown = false;
static FactoryUIPageID current_page;
static uint8_t current_tile = 0;

static lv_obj_t *voltage_label;
static lv_obj_t *percent_meter;
static lv_meter_indicator_t *percent_indic;
static bool touchDetected;


#include <vector>

std::vector<lv_timer_t *> timers;
SX1262 radio = new Module(LORA_CS, LORA_IRQ, LORA_RST, LORA_BUSY);

struct CameraFrame {
    lv_color_t *buf;
    lv_draw_img_dsc_t draw_dsc;
    lv_img_dsc_t img_dsc;
    lv_obj_t *img ;
} camera_frame;

LilyGo_Button bootPin;

enum TransmissionDirection {
    LORA_NONE = 0,
    TRANSMISSION = 1,
    RECEIVE = 2,
};

TransmissionDirection  transmissionDirection = LORA_NONE;

//! You can use EspTouch to configure the network key without changing the WiFi password below
#ifndef WIFI_SSID
#define WIFI_SSID             "Your WiFi SSID"
#endif
#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD         "Your WiFi PASSWORD"
#endif

#define WIFI_MSG_ID             0x1001

// Adjust the time server and corresponding event offset according to your own situation
#define NTP_SERVER1           "pool.ntp.org"
#define NTP_SERVER2           "time.nist.gov"

/**
 * The time zone is used by default. If you need to use an offset, please change the synchronization method in the setup function.
 * A more convenient approach to handle TimeZones with daylightOffset
 * would be to specify a environmnet variable with TimeZone definition including daylight adjustmnet rules.
 * A list of rules for your zone could be obtained from https://github.com/esp8266/Arduino/blob/master/cores/esp8266/TZ.h
 */

#define CFG_TIME_ZONE         "CST-8"       // TZ_Asia_Shanghai 

// #define GMT_OFFSET_SEC        0
// #define DAY_LIGHT_OFFSET_SEC  0


const char *week_char[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const char *month_char[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sept", "Oct", "Nov", "Dec"};

static void lv_gui_init();
static void lv_gui_select_next_item();
static void WiFiEvent(WiFiEvent_t event);
static void timeavailable(struct timeval *t);


void showMessageToScreen(String message)
{
    for (auto timer : timers) {
        lv_timer_del(timer);
    }
    timers.clear();

    lv_obj_clean(lv_scr_act());

    // Create display area 126 x 126
    lv_obj_t *cont = lv_obj_create(lv_scr_act());
    lv_obj_set_style_bg_color(cont, lv_color_black(), 0);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);
    // Set display window size
    lv_obj_set_size(cont, GlassViewableWidth, GlassViewableHeight);
    // Set window position
    lv_obj_align(cont, LV_ALIGN_BOTTOM_MID, 0, 0);

    lv_obj_t *label = lv_label_create(cont);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(label, message.c_str());

    int i = 5;
    while (i--) {
        lv_task_handler();
        delay(1000);
    }
}

void boot_button_event_callback(ButtonState state)
{
    Serial.print("BOOT Button ");
    switch (state) {
    case BTN_PRESSED_EVENT:
        Serial.println("pressed");
        break;
    case BTN_CLICK_EVENT:
        break;
    case BTN_LONG_PRESSED_EVENT:
        Serial.println("long Pressed\n");
#if 1
        showMessageToScreen("Sleep.");
        // Set touch button wake-up and set the touch threshold for wake-up
        glass.enableTouchWakeup(200);
        // Go to sleep
        glass.sleep();
#else

        if (!glass.ppm.isVbusIn()) {

            showMessageToScreen("Power OFF");

            glass.ppm.shutdown();

            shutdown = true;
        } else {
            // Set touch button wake-up and set the touch threshold for wake-up
            glass.enableTouchWakeup(200);
            // Go to sleep
            glass.sleep();
        }
#endif

        break;
    case BTN_DOUBLE_CLICK_EVENT:
        Serial.println("double click\n");
        break;
    case BTN_TRIPLE_CLICK_EVENT:
        Serial.println("triple click\n");
        break;
    default:
        break;
    }
}

static ICACHE_RAM_ATTR void setFlag(void)
{
    transmissionFlag = true;
}

static bool setLoRaParams()
{
    Serial.println("---------------------------------------");

    Serial.println("- Initializing LoRa radio module...-");
    Serial.printf("- Freq:%.2f MHZ\n", glass_setting.lora_freq);
    Serial.printf("- TxPower:%u dBm\n", glass_setting.lora_tx_power);
    Serial.printf("- Spreading Factor:%u \n", glass_setting.lora_sf);
    Serial.printf("- Bandwidth:%.2f kHz\n", glass_setting.lora_bw);
    Serial.printf("- Coding Rate:%u \n", glass_setting.lora_cr);
    Serial.printf("- Sync Word:0x%02X \n", glass_setting.lora_sw);
    Serial.printf("- Preamble Length:%u \n", glass_setting.lora_preamble_length);
    Serial.printf("- LoRa Tx Enable:%s \n", glass_setting.lora_tx_enable ? "true" : "false");
    Serial.printf("- Touch Enter Sleep:%s \n", glass_setting.touch_enter_sleep ? "true" : "false");
    Serial.println("---------------------------------------");

    radio.standby();

    if (radio.setFrequency(glass_setting.lora_freq) == RADIOLIB_ERR_INVALID_FREQUENCY) {
        Serial.println(F("Selected frequency is invalid for this module!"));
        return false;
    }

    if (radio.setBandwidth(glass_setting.lora_bw) == RADIOLIB_ERR_INVALID_BANDWIDTH) {
        Serial.println(F("Selected bandwidth is invalid for this module!"));
        return false;
    }

    if (radio.setSpreadingFactor(glass_setting.lora_sf) == RADIOLIB_ERR_INVALID_SPREADING_FACTOR) {
        Serial.println(F("Selected spreading factor is invalid for this module!"));
        return false;
    }

    if (radio.setCodingRate(glass_setting.lora_cr) == RADIOLIB_ERR_INVALID_CODING_RATE) {
        Serial.println(F("Selected coding rate is invalid for this module!"));
        return false;
    }

    if (radio.setSyncWord(glass_setting.lora_sw) != RADIOLIB_ERR_NONE) {
        Serial.println(F("Unable to set sync word!"));
        return false;
    }

    // set output power (accepted range is -17 - 22 dBm)
    if (radio.setOutputPower(glass_setting.lora_tx_power) == RADIOLIB_ERR_INVALID_OUTPUT_POWER) {
        Serial.println(F("Selected output power is invalid for this module!"));
        return false;
    }

    // set over current protection limit to 140 mA (accepted range is 45 - 140 mA)
    // NOTE: set value to 0 to disable overcurrent protection
    if (radio.setCurrentLimit(140) == RADIOLIB_ERR_INVALID_CURRENT_LIMIT) {
        Serial.println(F("Selected current limit is invalid for this module!"));
        return false;
    }

    // set LoRa preamble length to 15 symbols (accepted range is 0 - 65535)
    if (radio.setPreambleLength(glass_setting.lora_preamble_length) == RADIOLIB_ERR_INVALID_PREAMBLE_LENGTH) {
        Serial.println(F("Selected preamble length is invalid for this module!"));
        return false;
    }

    // disable CRC
    if (radio.setCRC(false) == RADIOLIB_ERR_INVALID_CRC_CONFIGURATION) {
        Serial.println(F("Selected CRC is invalid for this module!"));
        return false;
    }

    // set the function that will be called
    radio.setDio1Action(setFlag);

    if (transmissionDirection == TRANSMISSION) {
        Serial.println(F("Starting to transmit LoRa packets..."));
        int state = radio.startTransmit("Hello World!", 12);
        if (state == RADIOLIB_ERR_NONE) {
            Serial.println(F("Transmitting LoRa packet..."));
        } else if (state == RADIOLIB_ERR_SPI_CMD_TIMEOUT) {
            Serial.println(F("SPI communication with LoRa module timed out!"));
            return false;
        } else {
            Serial.print(F("Failed to start transmission: "));
            Serial.println(state);
            return false;
        }
    } else if (transmissionDirection == RECEIVE) {
        Serial.println(F("Starting to receive LoRa packets..."));
        int state = radio.startReceive();
        if (state == RADIOLIB_ERR_NONE) {
            Serial.println(F("Waiting for LoRa packet..."));
        } else if (state == RADIOLIB_ERR_SPI_CMD_TIMEOUT) {
            Serial.println(F("SPI communication with LoRa module timed out!"));
            return false;
        } else {
            Serial.print(F("Failed to start reception: "));
            Serial.println(state);
            return false;
        }
    }
    return true;
}

void setup()
{
    bool rslt = false;

    // Turn on debugging message output, Arduino IDE users please put
    // Tools -> USB CDC On Boot -> Enable, otherwise there will be no output
    Serial.begin(115200);

    // Get preferences
    bool res = preferences.begin("glass_config", false);
    if (!res) {
        Serial.println("Set default params");
        preferences.putBytes("lora_params", (const void *)&glass_setting, sizeof(glass_setting));
    } else {
        preferences.getBytes("lora_params", (void *)&glass_setting, sizeof(glass_setting));
        Serial.println("Get params from preferences:");
        Serial.printf("LoRa TX Enable: %d\n", glass_setting.lora_tx_enable);
        Serial.printf("LoRa TX Power: %d\n", glass_setting.lora_tx_power);
        Serial.printf("LoRa Preamble Length: %d\n", glass_setting.lora_preamble_length);
        Serial.printf("LoRa Bandwidth: %.2f\n", glass_setting.lora_bw);
        Serial.printf("LoRa Coding Rate: %d\n", glass_setting.lora_cr);
        Serial.printf("LoRa Sync Word: %d\n", glass_setting.lora_sw);
        Serial.printf("LoRa Frequency: %.2f\n", glass_setting.lora_freq);
        Serial.printf("Touch Threshold: %u\n", glass_setting.touch_threshold);
        Serial.printf("Touch Enter Sleep:%s \n", glass_setting.touch_enter_sleep ? "true" : "false");
    }
    preferences.end();


    // Initialization screen and peripherals
    rslt = glass.begin();
    if (!rslt) {
        while (1) {
            Serial.println("The board model cannot be detected, please raise the Core Debug Level to an error");
            delay(1000);
        }
    }

    int state = radio.begin();
    if (state == RADIOLIB_ERR_NONE) {
        Serial.printf("Radio initialized successfully\n");
    } else {
        Serial.printf("Radio initialization failed\n");
    }

    // Initialize boot pin
    bootPin.init(BOARD_BOOT_PIN);

    // Set boot button callback function
    bootPin.setEventCallback(boot_button_event_callback);

    // Brightness range : 0 ~ 255
    glass.setBrightness(255);

    // Initialize lvgl
    beginLvglHelper(glass);

    // Touch button interrupt
    attachInterrupt(digitalPinToInterrupt(BOARD_TOUCH_BUTTON), []() {
        touchDetected = true;
    }, FALLING);

    // Set notification call-back function , After time synchronization is completed, synchronize the synchronized time to the hardware RTC
    sntp_set_time_sync_notification_cb( timeavailable );

    /**
    * This will set configured ntp servers and constant TimeZone/daylightOffset
    * should be OK if your time zone does not need to adjust daylightOffset twice a year,
    * in such a case time adjustment won't be handled automagicaly.
    */
    // configTime(GMT_OFFSET_SEC, DAY_LIGHT_OFFSET_SEC, NTP_SERVER1, NTP_SERVER2);

    /**
     * A more convenient approach to handle TimeZones with daylightOffset
     * would be to specify a environmnet variable with TimeZone definition including daylight adjustmnet rules.
     * A list of rules for your zone could be obtained from https://github.com/esp8266/Arduino/blob/master/cores/esp8266/TZ.h
     */
    configTzTime(CFG_TIME_ZONE, NTP_SERVER1, NTP_SERVER2);


    // Check WiFi credentials
    if (String(WIFI_SSID) == "Your WiFi SSID" || String(WIFI_PASSWORD) == "Your WiFi PASSWORD" ) {
        Serial.println("[Error] : WiFi ssid and password are not configured correctly");
        Serial.println("[Error] : WiFi ssid and password are not configured correctly");
        Serial.println("[Error] : WiFi ssid and password are not configured correctly");

        WiFi.mode(WIFI_AP);
        WiFi.softAP("T-Glass-Factory");
        Serial.println("[WiFi]: SoftAP started");
        Serial.println("[WiFi]: Connect to T-Glass-Factory");
        Serial.println("[WiFi]: IP address: " + WiFi.softAPIP().toString());

    } else {
        // For factory WiFi connection testing only
        Serial.print("Use default WiFi SSID & PASSWORD!!");
        Serial.print("SSID:"); Serial.println(WIFI_SSID);
        Serial.print("PASSWORD:"); Serial.println(WIFI_PASSWORD);
        // Initialize WiFi
        WiFi.mode(WIFI_STA);
        WiFi.onEvent(WiFiEvent);    // Register WiFi event
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            Serial.print(".");
        }
        Serial.println("");
        Serial.println("WiFi connected");

        // extern void startCameraServer();
        // startCameraServer();
        // Serial.print("Camera Ready! Use 'http://");
        // Serial.print(WiFi.localIP());
        // Serial.println("' to connect");
        // while(1){
        //     delay(100000);
        // }

    }

    // Initialize factory gui
    lv_gui_init();

    setLoRaParams();

    radio.sleep(true);;

    Serial.println("Enter loop.");

}

void handleWomCommands()
{
    if (!Serial.available()) {
        return;
    }

    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd.length() == 0) {
        return;
    }

    int colonPos = cmd.indexOf(':');
    String key = (colonPos == -1) ? cmd : cmd.substring(0, colonPos);
    String valStr = (colonPos == -1) ? "" : cmd.substring(colonPos + 1);
    valStr.trim();

    bool paramChanged = false;

    if (key == "sf") {
        // Spreading Factor 6~12
        if (valStr.length() == 0) {
            Serial.println("Error: missing value for sf");
        } else {
            int sf = valStr.toInt();
            if (sf == 0 && valStr != "0") {
                Serial.println("Error: invalid spreading factor (must be integer)");
            } else if (sf >= 6 && sf <= 12) {
                glass_setting.lora_sf = sf;
                paramChanged = true;
                Serial.printf("Spreading Factor set to %u\n", glass_setting.lora_sf);
            } else {
                Serial.printf("Error: spreading factor must be 6~12 (got %d)\n", sf);
            }
        }
    } else if (key == "bw") {
        // Bandwidth: 62.5, 125.0, 250.0 kHz
        if (valStr.length() == 0) {
            Serial.println("Error: missing value for bw");
        } else {
            float bw = valStr.toFloat();
            if (bw == 0.0f && valStr != "0" && valStr != "0.0") {
                Serial.println("Error: invalid bandwidth (must be number)");
            } else {
                bool valid = (fabs(bw - 62.5f) < 0.01f) ||
                             (fabs(bw - 125.0f) < 0.01f) ||
                             (fabs(bw - 250.0f) < 0.01f);
                if (valid) {
                    glass_setting.lora_bw = bw;
                    paramChanged = true;
                    Serial.printf("Bandwidth set to %.2f kHz\n", glass_setting.lora_bw);
                } else {
                    Serial.printf("Error: bandwidth must be 62.5, 125.0 or 250.0 kHz (got %.2f)\n", bw);
                }
            }
        }
    } else if (key == "sw") {
        if (valStr.length() == 0) {
            Serial.println("Error: missing value for sw");
        } else {
            int sw = valStr.toInt();
            if (sw == 0 && valStr != "0") {
                Serial.println("Error: invalid sw value (must be integer)");
            } else if (sw >= 0 && sw <= 255) {
                glass_setting.lora_sw = sw;   // 确保 glass_setting 有 lora_sw 字段
                paramChanged = true;
                Serial.printf("SW set to %u\n", glass_setting.lora_sw);
            } else {
                Serial.printf("Error: sw must be 0~255 (got %d)\n", sw);
            }
        }
    } else if (key == "cr") {
        // Coding Rate: 4~6
        if (valStr.length() == 0) {
            Serial.println("Error: missing value for cr");
        } else {
            int cr = valStr.toInt();
            if (cr == 0 && valStr != "0") {
                Serial.println("Error: invalid coding rate (must be integer)");
            } else if (cr >= 4 && cr <= 6) {
                glass_setting.lora_cr = cr;
                paramChanged = true;
                Serial.printf("Coding Rate set to %u\n", glass_setting.lora_cr);
            } else {
                Serial.printf("Error: coding rate must be 4~6 (got %d)\n", cr);
            }
        }
    } else if (key == "tp") {
        // Tx Power 0~22 dBm
        if (valStr.length() == 0) {
            Serial.println("Error: missing value for tp");
        } else {
            int tp = valStr.toInt();
            if (tp == 0 && valStr != "0") {
                Serial.println("Error: invalid tx power (must be integer)");
            } else if (tp >= 0 && tp <= 22) {
                glass_setting.lora_tx_power = tp;
                paramChanged = true;
                Serial.printf("Tx Power set to %u dBm\n", glass_setting.lora_tx_power);
            } else {
                Serial.printf("Error: tx power must be 0~22 dBm (got %d)\n", tp);
            }
        }
    } else if (key == "freq") {
        // Frequency 433.0 ~ 923.0 MHz
        if (valStr.length() == 0) {
            Serial.println("Error: missing value for freq");
        } else {
            float freq = valStr.toFloat();
            if (freq == 0.0f && valStr != "0" && valStr != "0.0") {
                Serial.println("Error: invalid frequency (must be number)");
            } else if (freq >= 433.0f && freq <= 923.0f) {
                glass_setting.lora_freq = freq;
                paramChanged = true;
                Serial.printf("Frequency set to %.2f MHz\n", glass_setting.lora_freq);
            } else {
                Serial.printf("Error: frequency must be 433.0~923.0 MHz (got %.2f)\n", freq);
            }
        }
    }  else if (key == "pl") {
        // Preamble Length 0 ~ 65535
        if (valStr.length() == 0) {
            Serial.println("Error: missing value for preamble length");
        } else {
            int pl = valStr.toInt();
            if (pl == 0 && valStr != "0") {
                Serial.println("Error: invalid preamble length (must be integer)");
            } else if (pl >= 0 && pl <= 65535) {
                paramChanged = true;
                Serial.printf("Preamble Length set to %d\n", pl);
                glass_setting.lora_preamble_length = pl;
            } else {
                Serial.printf("Error: preamble length must be 0~65535 (got %d)\n", pl);
            }
        }
    } else if (key == "tx") {
        // LoRa Tx Enable
        if (valStr.length() == 0) {
            Serial.println("Error: missing value for tx");
        } else {
            paramChanged = true;
            bool tx = (valStr == "1");
            glass_setting.lora_tx_enable = tx;
            Serial.printf("LoRa Tx Enable set to %s\n", tx ? "true" : "false");
        }
    } else if (key == "sleep") {
        // Sleep mode
        if (valStr.length() == 0) {
            Serial.println("Error: missing value for sleep");
        } else {
            paramChanged = true;
            bool sleep = (valStr == "1");
            glass_setting.touch_enter_sleep = sleep;
            Serial.printf("Touch enter sleep %s\n", sleep ? "true" : "false");
        }
    }

    else if (key == "wifi") {
        WiFi.mode(WIFI_STA);
        WiFi.disconnect();
        int n = WiFi.scanNetworks();
        Serial.println("scan done");
        if (n == 0) {
            Serial.println("no networks found");
        } else {
            Serial.print(n);
            Serial.println(" networks found");
            for (int i = 0; i < n; ++i) {
                Serial.print(i + 1);
                Serial.print(": ");
                Serial.print(WiFi.SSID(i));
                Serial.print(" (");
                Serial.print(WiFi.RSSI(i));
                Serial.print(")");
                Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
                delay(10);
            }
        }
        Serial.println("");
        WiFi.mode(WIFI_OFF);
    } else if (key == "send") {
        Serial.println("LoRa sent...");
        lv_obj_set_tile_id(tileview, PAGE_LORA_TX, 0, LV_ANIM_OFF);
    } else if (key == "recv") {
        Serial.println("LoRa recv...");
        lv_obj_set_tile_id(tileview, PAGE_LORA_RX, 0, LV_ANIM_OFF);
    } else if (key == "next") {
        Serial.print(lv_obj_get_index(lv_tileview_get_tile_act(tileview)));
        lv_gui_select_next_item();
    } else if (key == "touchRead") {
        Serial.println("Touch read:");
        uint32_t value = touchRead(BOARD_TOUCH_BUTTON);
        Serial.println(value);
    } else {
        Serial.println("Unknown command");
    }

    if (paramChanged) {
        if (setLoRaParams()) {
            Serial.println("LoRa parameters updated successfully");
        } else {
            Serial.println("Failed to update LoRa parameters");
        }

        preferences.begin("glass_config", false);
        preferences.putBytes("lora_params", (const void *)&glass_setting, sizeof(glass_setting));
        preferences.end();
    }
}
int transmissionState;

void loop()
{
    if (touchDetected) {
        touchDetected = false;
        Serial.println("Touch pressed");
        lv_gui_select_next_item();
    }

    handleWomCommands();

    switch (current_page) {
    case PAGE_CAMERA: {
        if (glass.isCameraDetected()) {
            camera_fb_t *frame = esp_camera_fb_get();
            if (frame) {
                if (frame->format == PIXFORMAT_RGB565) {
                    memcpy(camera_frame.buf, frame->buf, frame->len);

                    camera_frame.img_dsc.header.cf = LV_IMG_CF_TRUE_COLOR;
                    camera_frame.img_dsc.header.always_zero = 0;
                    camera_frame.img_dsc.header.w = frame->width;
                    camera_frame.img_dsc.header.h = frame->height;
                    camera_frame.img_dsc.data_size = frame->width * frame->height * sizeof(lv_color_t);
                    camera_frame.img_dsc.data = (const uint8_t *)camera_frame.buf;

                    lv_img_set_src(camera_frame.img, &camera_frame.img_dsc);
                    lv_obj_set_size(camera_frame.img, frame->width, frame->height);
                }
                esp_camera_fb_return(frame);
            }
        }
    }
    break;
    case PAGE_MIC_LEVEL: {
        static uint32_t interval = 0;
        int left, right;
        if (millis() > interval) {
            interval = millis() + 200;
            glass.getAudioLevels(&left, &right);
            lv_bar_set_value(audio_level_left_bar, left, LV_ANIM_OFF);
            lv_bar_set_value(audio_level_right_bar, right, LV_ANIM_OFF);
        }
    }
    break;
    case PAGE_DATETIME:
        update_datetime();
        break;
    case PAGE_LORA_TX: {

        if ( glass_setting.lora_tx_enable ) {
            static uint32_t interval = 0;
            static uint32_t counter = 0;

            if (millis() - interval > 1000) {
                if (transmissionDirection != TRANSMISSION) {
                    // If not in transmission mode, start transmission
                    Serial.printf("[%08u]:", millis() / 1000);
                    Serial.println("Start LoRa Transmission.");
                    transmissionState = radio.startTransmit("First");
                    if (transmissionState != RADIOLIB_ERR_NONE) {
                        Serial.printf("LoRa Transmit Error: %d\n", transmissionState);
                        radio.standby();
                    } else {
                        transmissionDirection = TRANSMISSION;
                        Serial.println("LoRa Transmit Success");
                    }
                }
                if (transmissionFlag) {
                    transmissionFlag = false;
                    String payload = "Hello #" + String(counter++);
                    int16_t res = radio.startTransmit(payload);
                    if (res != RADIOLIB_ERR_NONE) {
                        Serial.print("LoRa Transmit Error: ");
                        Serial.println(res);
                    }
                    lv_label_set_text_fmt(lora_tx_message_label, "[%c]:%s", res == RADIOLIB_ERR_NONE ? 'O' : 'X', payload.c_str());
                }

                interval = millis();
            }
        }
    }
    break;
    case PAGE_LORA_RX:
#if 1
        if (transmissionDirection != RECEIVE) {
            transmissionDirection = RECEIVE;
            Serial.printf("[%08u]:", millis() / 1000);
            Serial.println("Start LoRa Receive.");
            int state = radio.startReceive();
            if (state != RADIOLIB_ERR_NONE) {
                Serial.printf("LoRa Receive Error: %d\n", state);
            } else {
                Serial.println("LoRa Receive Success");
            }
        }

        // check if the flag is set
        if (transmissionFlag) {

            String recv;

            // reset flag
            transmissionFlag = false;

            // you can read received data as an Arduino String
            // int state = radio.readData(recv);

            // you can also read received data as byte array
            /*
            */
            int state = radio.readData(recv);
            if (state == RADIOLIB_ERR_NONE) {

                Serial.printf("[%08u]:", millis() / 1000);

                // packet was successfully received
                Serial.print(F(" Received packet!"));

                // print data of the packet
                Serial.print(F(" Data:"));
                Serial.print(recv);

                // print RSSI (Received Signal Strength Indicator)
                Serial.print(F(" RSSI:"));
                Serial.print(radio.getRSSI());
                Serial.print(F(" dBm"));

                // print SNR (Signal-to-Noise Ratio)
                Serial.print(F("  SNR:"));
                Serial.print(radio.getSNR());
                Serial.println(F(" dB"));
                lv_label_set_text_fmt(lora_rx_message_label, "RX:%s\nRSSI:%.2f\nSNR:%.2f", recv.c_str(), radio.getRSSI(), radio.getSNR());

            } else if (state ==  RADIOLIB_ERR_CRC_MISMATCH) {
                // packet was received, but is malformed
                Serial.println(F("CRC error!"));
            } else {
                // some other error occurred
                Serial.print(F("failed, code "));
                Serial.println(state);

                lv_label_set_text_fmt(lora_rx_message_label, "Rx Failed:%d", state);

            }
            // put module back to listen mode
            radio.startReceive();

        }
#endif
        break;
    case PAGE_BATTERY_VOLTAGE: {
        // Obtain battery voltage, based on ADC reading, there is a certain error
        uint16_t battery_voltage = glass.getBattVoltage();
        // Calculate battery percentage.
        int percentage = glass.getBatteryPercent();
        lv_label_set_text_fmt(voltage_label, "%.2f", battery_voltage / 1000.0);
        lv_meter_set_indicator_end_value(percent_meter, percent_indic, percentage);
    }
    break;
    case PAGE_WIFI: {
        if (!WiFi.isConnected()) {
            lv_label_set_text(wifi_label_rssi, "N/A");
        } else {
            int32_t rssi = WiFi.RSSI();
            lv_label_set_text_fmt(wifi_label_rssi, "%d",  rssi);
            lv_meter_set_indicator_end_value(wifi_rssi_meter, wifi_rssi_indic2, rssi);
        }
    }
    break;
    case PAGE_IMAGE:
        break;
    case PAGE_NOISE_DETECT:
        break;
    default:
        break;
    }

    // Update 6-axis sensor and button state
    glass.update();
    // Update BOOT state
    bootPin.update();

    lv_timer_handler();
    delay(1);
}

// Callback function (get's called when time adjusts via NTP)
static void timeavailable(struct timeval * t)
{
    Serial.println("Got time adjustment from NTP!");
    // Synchronize the synchronized time to the hardware RTC
    // glass.hwClockWrite();
}


static void update_datetime()
{
    static uint32_t update_interval = 0;
    struct tm timeinfo;
    if (millis() > update_interval) {
        update_interval = millis() + 500;
        time_t now;
        time(&now);
        localtime_r(&now, &timeinfo);

        // Here need to test the hardware time, so get the time in the RTC PCF85063
        // glass.getDateTime(&timeinfo);

        lv_label_set_text_fmt(time_label, "%02d%s%02d", timeinfo.tm_hour, colon != 0 ? "#ffffff :#" : "#000000 :#", timeinfo.tm_min);
        colon = !colon;
        lv_label_set_text_fmt(week_label, "%s", week_char[timeinfo.tm_wday]);
        lv_label_set_text_fmt(month_label, "%s", month_char[timeinfo.tm_mon]);
    }

}

static void lv_tileview_add_camera_frame(lv_obj_t *parent)
{
    if (glass.isCameraDetected()) {
        camera_frame.img = lv_img_create(parent);
        lv_obj_center(camera_frame.img);
        camera_fb_t *frame = esp_camera_fb_get();
        if (frame != NULL) {
            camera_frame.buf = (lv_color_t *)ps_malloc(frame->width * frame->height * sizeof(lv_color_t));
            Serial.printf("Camera init: %dx%d format: %d\n",  frame->width, frame->height, frame->format);
            esp_camera_fb_return(frame);
            return;
        }
    }
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_style_text_color(label, font_color, LV_PART_MAIN);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_label_set_text(label, "Camera is offline");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
}

static void lv_tileview_add_audio_level(lv_obj_t *parent)
{
    audio_level_label = lv_label_create(parent);
    lv_label_set_long_mode(audio_level_label, LV_LABEL_LONG_WRAP);
    lv_label_set_recolor(audio_level_label, true);
    lv_obj_set_style_text_color(audio_level_label, font_color, LV_PART_MAIN);
    lv_obj_set_style_text_font(audio_level_label, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_label_set_text(audio_level_label, "Mic Level");
    lv_obj_align(audio_level_label, LV_ALIGN_CENTER, 0, 0);

    audio_level_left_bar = lv_bar_create(parent);
    lv_obj_set_width(audio_level_left_bar, LV_PCT(100));
    lv_obj_set_height(audio_level_left_bar, LV_PCT(10));
    lv_obj_align_to(audio_level_left_bar, audio_level_label, LV_ALIGN_BOTTOM_MID, 0, 15);
    lv_bar_set_range(audio_level_left_bar, 100, 10000);

    audio_level_right_bar = lv_bar_create(parent);
    lv_obj_set_width(audio_level_right_bar, LV_PCT(100));
    lv_obj_set_height(audio_level_right_bar, LV_PCT(10));
    lv_obj_align_to(audio_level_right_bar, audio_level_left_bar, LV_ALIGN_BOTTOM_MID, 0, 15);
    lv_bar_set_range(audio_level_right_bar, 100, 10000);
}

static void lv_tileview_add_datetime(lv_obj_t *parent)
{
    lv_obj_set_scroll_dir(parent, LV_DIR_NONE);
    // TIME
    lv_obj_t *time_cont = lv_obj_create(parent);
    lv_obj_set_size(time_cont, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(time_cont, bg_color, 0);
    lv_obj_set_style_border_width(time_cont, 0, 0);
    lv_obj_set_scrollbar_mode(time_cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(time_cont, LV_DIR_NONE);

    time_label = lv_label_create(time_cont);
    lv_label_set_recolor(time_label, 1);
    lv_label_set_text(time_label, "12:34");
    lv_obj_set_style_text_color(time_label, font_color, 0);
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_24, 0);
    lv_obj_align(time_label, LV_ALIGN_CENTER, 0, -15);

    week_label = lv_label_create(time_cont);
    lv_obj_set_style_text_color(week_label, font_color, 0);
    lv_obj_set_style_text_font(week_label, &lv_font_montserrat_10, 0);
    lv_label_set_text(week_label, "Thu");
    lv_obj_align_to(week_label, time_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);

    month_label = lv_label_create(time_cont);
    lv_obj_set_style_text_color(month_label, font_color, 0);
    lv_obj_set_style_text_font(month_label, &lv_font_montserrat_10, 0);
    lv_label_set_text(month_label, "Feb");
    lv_obj_align_to(month_label, time_label, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 5);

}

static void lv_tileview_add_lora_tx(lv_obj_t *parent)
{
    lv_obj_t *title = lv_label_create(parent);
    // lv_label_set_long_mode(title, LV_LABEL_LONG_WRAP); /*Break the long lines*/
    // lv_label_set_recolor(title, true);                 /*Enable re-coloring by commands in the text*/
    lv_obj_set_style_text_color(title, font_color, LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_label_set_text(title, "LoRa Tx");
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    lora_tx_message_label = lv_label_create(parent);
    lv_label_set_text(lora_tx_message_label, "N.A");
    lv_obj_set_style_text_color(lora_tx_message_label, font_color, LV_PART_MAIN);
    lv_obj_set_style_text_font(lora_tx_message_label, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_align(lora_tx_message_label, LV_ALIGN_CENTER, 0, 15);

}

static void lv_tileview_add_lora_rx(lv_obj_t *parent)
{
    lv_obj_t *title = lv_label_create(parent);
    // lv_label_set_long_mode(title, LV_LABEL_LONG_WRAP); /*Break the long lines*/
    // lv_label_set_recolor(title, true);                 /*Enable re-coloring by commands in the text*/
    lv_obj_set_style_text_color(title, font_color, LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_label_set_text(title, "LoRa Rx");
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -10);

    lora_rx_message_label = lv_label_create(parent);
    lv_label_set_text(lora_rx_message_label, "N.A");
    lv_obj_set_style_text_color(lora_rx_message_label, font_color, LV_PART_MAIN);
    lv_obj_set_style_text_font(lora_rx_message_label, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_align(lora_rx_message_label, LV_ALIGN_CENTER, 0, 20);

}


static void lv_tileview_add_wifi(lv_obj_t *parent)
{

    static lv_obj_t *label;
    static lv_meter_indicator_t *indic1 ;

    wifi_rssi_meter = lv_meter_create(parent);

    /*Remove the background and the circle from the middle*/
    lv_obj_remove_style(wifi_rssi_meter, NULL, LV_PART_MAIN);
    lv_obj_remove_style(wifi_rssi_meter, NULL, LV_PART_INDICATOR);

    lv_obj_set_size(wifi_rssi_meter, LV_PCT(50), LV_PCT(50));
    lv_obj_center(wifi_rssi_meter);

    /*Add a scale first with no ticks.*/
    lv_meter_scale_t *scale = lv_meter_add_scale(wifi_rssi_meter);
    lv_meter_set_scale_ticks(wifi_rssi_meter, scale, 0, 0, 0, lv_color_black());
    lv_meter_set_scale_range(wifi_rssi_meter, scale, -100, 10, 280, 130);

    /*Add a three arc indicator*/
    lv_coord_t indic_w = 15;

    indic1 = lv_meter_add_arc(wifi_rssi_meter, scale, indic_w, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_meter_set_indicator_start_value(wifi_rssi_meter, indic1, 0);
    lv_meter_set_indicator_end_value(wifi_rssi_meter, indic1, 100);

    wifi_rssi_indic2 = lv_meter_add_arc(wifi_rssi_meter, scale, indic_w, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_meter_set_indicator_start_value(wifi_rssi_meter, wifi_rssi_indic2, 0);
    lv_meter_set_indicator_end_value(wifi_rssi_meter, wifi_rssi_indic2, 90);


    wifi_label_rssi = lv_label_create(parent);
    lv_obj_set_style_text_color(wifi_label_rssi, lv_color_white(), 0);
    lv_label_set_text(wifi_label_rssi, "0");
    lv_obj_set_style_text_font(wifi_label_rssi, &lv_font_montserrat_12, 0);
    lv_obj_center(wifi_label_rssi);

    label = lv_label_create(parent);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_label_set_text(label, "RSSI");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
    lv_obj_align_to(label, wifi_label_rssi, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
}

static void ofs_y_anim(void *img, int32_t v)
{
    lv_img_set_offset_y((lv_obj_t *)img, v);
}

static void ofs_x_anim(void *img, int32_t v)
{
    lv_img_set_offset_x((lv_obj_t *)img, v);
}

typedef void (*offset_cb)(void *img, int32_t v);
static const char *str[] = {"backward", "left", "right", "forward"};
static const void *ptr_img[]  = {&img_down, &img_left, &img_right, &img_up};
static const offset_cb ptr_cb[]  = {ofs_y_anim, ofs_x_anim, ofs_x_anim, ofs_y_anim};

// Just for testing images
static void lv_tileview_add_img(lv_obj_t *parent)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(cont, bg_color, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(cont, LV_DIR_NONE);

    lv_obj_t *img = lv_img_create(cont);
    lv_img_set_src(img, &img_down);
    lv_obj_center(img);

    lv_obj_t *label = lv_label_create(cont);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(label, font_color, 0);
    lv_label_set_text(label, str[0]);
    lv_obj_align_to(label, img, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);


    lv_obj_set_user_data(label, img);

    lv_timer_t *timer =  lv_timer_create([](lv_timer_t *t) {

        static int i = 0;

        lv_obj_t *label =   (lv_obj_t *)t->user_data;
        lv_obj_t *img = (lv_obj_t *)lv_obj_get_user_data(label);

        lv_anim_del(img, NULL);

        lv_img_set_src(img, ptr_img[i]);
        lv_obj_center(img);

        lv_img_set_offset_x(img, 0);
        lv_img_set_offset_y(img, 0);

        lv_label_set_text_fmt(label, "%s",  str[i]);
        lv_obj_align_to(label, img, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);

        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, img);
        lv_anim_set_exec_cb(&a, ptr_cb[i]);

        switch (i) {
        case 0:
            lv_anim_set_values(&a, 0, 100);
            break;
        case 1:
            lv_anim_set_values(&a, 100, 0);
            break;
        case 2:
            lv_anim_set_values(&a, 0, 100);
            break;
        case 3:
            lv_anim_set_values(&a, 100, 0);
            break;
        default:
            break;
        }
        lv_anim_set_time(&a, 3000);
        lv_anim_set_playback_time(&a, 500);
        lv_anim_set_repeat_count(&a, 3000);
        lv_anim_start(&a);
        i++;
        i %= (sizeof(str) / sizeof(str[0]));
    }, 3000, label);

    timers.push_back(timer);
}


static void lv_tileview_add_battery_voltage(lv_obj_t *parent)
{
    lv_obj_t *label;
    lv_meter_indicator_t *indic1;

    percent_meter = lv_meter_create(parent);

    /*Remove the background and the circle from the middle*/
    lv_obj_remove_style(percent_meter, NULL, LV_PART_MAIN);
    lv_obj_remove_style(percent_meter, NULL, LV_PART_INDICATOR);

    lv_obj_set_size(percent_meter, LV_PCT(50), LV_PCT(50));
    lv_obj_center(percent_meter);

    /*Add a scale first with no ticks.*/
    lv_meter_scale_t *scale = lv_meter_add_scale(percent_meter);
    lv_meter_set_scale_ticks(percent_meter, scale, 0, 0, 0, lv_color_black());
    lv_meter_set_scale_range(percent_meter, scale, 0, 100, 280, 130);

    /*Add a three arc indicator*/
    lv_coord_t indic_w = 15;

    indic1 = lv_meter_add_arc(percent_meter, scale, indic_w, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_meter_set_indicator_start_value(percent_meter, indic1, 0);
    lv_meter_set_indicator_end_value(percent_meter, indic1, 100);

    percent_indic = lv_meter_add_arc(percent_meter, scale, indic_w, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_meter_set_indicator_start_value(percent_meter, percent_indic, 0);
    lv_meter_set_indicator_end_value(percent_meter, percent_indic, 90);


    voltage_label = lv_label_create(parent);
    lv_obj_set_style_text_color(voltage_label, lv_color_white(), 0);
    lv_label_set_text(voltage_label, "0");
    lv_obj_set_style_text_font(voltage_label, &lv_font_montserrat_12, 0);
    lv_obj_center(voltage_label);

    label = lv_label_create(parent);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_label_set_text(label, "Volts");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
    lv_obj_align_to(label, voltage_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);

}

static void tileview_change_cb(lv_event_t *e)
{
    int state = 0;
    lv_obj_t *tileview = lv_event_get_target(e);

    current_page = (FactoryUIPageID )lv_obj_get_index(lv_tileview_get_tile_act(tileview));

    glass.tone();

    switch (current_page) {
    case PAGE_CAMERA:
        break;
    case PAGE_MIC_LEVEL:
        glass.initI2S();
        break;
    case PAGE_DATETIME:
        glass.deinitI2S();
        break;
    case PAGE_LORA_TX:
        break;
    case PAGE_LORA_RX:
        break;
    case PAGE_BATTERY_VOLTAGE:
        break;
    case PAGE_WIFI:
        break;
    case PAGE_IMAGE:
        break;
    }
}

static uint8_t get_current_tile()
{
    return current_tile;
}

static bool is_on_tile(int tile)
{
    return current_tile == tile;
}

static void lv_gui_select_next_item()
{
    uint32_t  page_id =  lv_obj_get_index(lv_tileview_get_tile_act(tileview));
    uint32_t  max_page_num = lv_obj_get_child_cnt(tileview) - 1;
    current_tile = max_page_num;
    page_id++;
    page_id %= max_page_num;
    lv_obj_set_tile_id(tileview, page_id, 0, LV_ANIM_ON);
}

static void lv_gui_init()
{
    // Set the page to all black
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), 0);

    // Create display area 126 x 126
    tileview = lv_tileview_create(lv_scr_act());
    lv_obj_set_style_bg_color(tileview, lv_color_black(), 0);
    lv_obj_set_scrollbar_mode(tileview, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_event_cb(tileview, tileview_change_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // Set display window size
    lv_obj_set_size(tileview, GlassViewableWidth, GlassViewableHeight);
    // Set window position
    lv_obj_align(tileview, LV_ALIGN_BOTTOM_MID, 0, 0);

    // Create the displayed UI
    lv_obj_t *t0 = lv_tileview_add_tile(tileview, PAGE_CAMERA, 0, LV_DIR_HOR | LV_DIR_BOTTOM);
    lv_obj_t *t1 = lv_tileview_add_tile(tileview, PAGE_MIC_LEVEL, 0, LV_DIR_HOR | LV_DIR_BOTTOM);
    lv_obj_t *t2 = lv_tileview_add_tile(tileview, PAGE_DATETIME, 0, LV_DIR_HOR | LV_DIR_BOTTOM);
    lv_obj_t *t3 = lv_tileview_add_tile(tileview, PAGE_LORA_TX, 0, LV_DIR_HOR | LV_DIR_BOTTOM);
    lv_obj_t *t4 = lv_tileview_add_tile(tileview, PAGE_LORA_RX, 0, LV_DIR_HOR | LV_DIR_BOTTOM);
    lv_obj_t *t5 = lv_tileview_add_tile(tileview, PAGE_BATTERY_VOLTAGE, 0,  LV_DIR_HOR | LV_DIR_BOTTOM);
    lv_obj_t *t6 = lv_tileview_add_tile(tileview, PAGE_WIFI, 0,  LV_DIR_HOR | LV_DIR_BOTTOM);
    lv_obj_t *t7 = lv_tileview_add_tile(tileview, PAGE_IMAGE, 0,  LV_DIR_HOR | LV_DIR_BOTTOM);

    // Create camera frame
    lv_tileview_add_camera_frame(t0);

    lv_tileview_add_audio_level(t1);

    // Create date page
    lv_tileview_add_datetime(t2);

    // Create sensor page
    lv_tileview_add_lora_tx(t3);

    lv_tileview_add_lora_rx(t4);

    // Create battery voltage page
    lv_tileview_add_battery_voltage(t5);

    // Create a WiFi quality page
    lv_tileview_add_wifi(t6);

    // Create image page
    lv_tileview_add_img(t7);

}


static void WiFiEvent(WiFiEvent_t event)
{
    Serial.printf("[WiFi-event] event: %d\n", event);
    switch (event) {
    case ARDUINO_EVENT_WIFI_READY:
        Serial.println("WiFi interface ready");
        break;
    case ARDUINO_EVENT_WIFI_SCAN_DONE:
        Serial.println("Completed scan for access points");
        break;
    case ARDUINO_EVENT_WIFI_STA_START:
        Serial.println("WiFi client started");
        break;
    case ARDUINO_EVENT_WIFI_STA_STOP:
        Serial.println("WiFi clients stopped");
        break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
        Serial.println("Connected to access point");
        lv_msg_send(WIFI_MSG_ID, NULL);
        break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        Serial.println("Disconnected from WiFi access point");
        lv_msg_send(WIFI_MSG_ID, NULL);
        break;
    case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE:
        Serial.println("Authentication mode of access point has changed");
        break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        Serial.print("Obtained IP address: ");
        Serial.println(WiFi.localIP());
        lv_msg_send(WIFI_MSG_ID, NULL);
        break;
    case ARDUINO_EVENT_WIFI_STA_LOST_IP:
        Serial.println("Lost IP address and IP address is reset to 0");
        lv_msg_send(WIFI_MSG_ID, NULL);
        break;
    default: break;
    }
}
