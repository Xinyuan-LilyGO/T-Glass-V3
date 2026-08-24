/**
 * @file      LoRa_868_915_PingPong.ino
 * @brief     BOOT-selectable SX1262 transmitter/receiver for T-Glass V3.
 *
 * The board starts in receive mode. Click BOOT to switch between TX and RX.
 * TX mode only transmits periodically; RX mode only listens for packets.
 */

#include <LilyGo_GlassV3.h>
#include <LV_Helper.h>

// Set both boards to the same legal frequency: 868.0 or 915.0 MHz.
#ifndef LORA_FREQUENCY_MHZ
#define LORA_FREQUENCY_MHZ 915.0
#endif

#define LORA_BANDWIDTH_KHZ 125.0
#define LORA_SPREADING_FACTOR 9
#define LORA_CODING_RATE 7
#define LORA_SYNC_WORD 0x12
#define LORA_TX_POWER_DBM 10
#define LORA_PREAMBLE_LENGTH 8
#define LORA_TX_INTERVAL_MS 1500

#define GLASS_VIEWABLE_WIDTH 126
#define GLASS_VIEWABLE_HEIGHT 126

SX1262 radio = new Module(LORA_CS, LORA_IRQ, LORA_RST, LORA_BUSY);
LilyGo_Button bootButton;

enum OperatingMode
{
    MODE_RECEIVE,
    MODE_TRANSMIT,
};

static OperatingMode operatingMode = MODE_RECEIVE;
static volatile bool radioOperationDone;
static bool modeSwitchRequested;
static bool radioActive;
static uint32_t nextActionAt;
static uint32_t txCounter;
static String lastTxPayload = "--";
static char nodeName[12];

static lv_obj_t *headerLabel;
static lv_obj_t *paramsLabel;
static lv_obj_t *txLabel;
static lv_obj_t *rxLabel;
static lv_obj_t *rssiLabel;
static lv_obj_t *snrLabel;
static lv_obj_t *statusLabel;

static void startReceive();
static void startTransmit();

void onRadioOperationDone()
{
    radioOperationDone = true;
}

void onBootButtonEvent(ButtonState state)
{
    if (state == BTN_CLICK_EVENT)
    {
        modeSwitchRequested = true;
    }
}

static lv_obj_t *createLine(lv_obj_t *parent, int16_t y, const lv_font_t *font, lv_color_t color)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_width(label, 120);
    lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, color, 0);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, y);
    return label;
}

static void updateModeUi()
{
    if (operatingMode == MODE_TRANSMIT)
    {
        lv_label_set_text_fmt(headerLabel, "LoRa TX %.1f", (double)LORA_FREQUENCY_MHZ);
        lv_obj_set_style_text_color(headerLabel, lv_palette_main(LV_PALETTE_GREEN), 0);
        lv_label_set_text(statusLabel, "TX mode / BOOT:RX");
    }
    else
    {
        lv_label_set_text_fmt(headerLabel, "LoRa RX %.1f", (double)LORA_FREQUENCY_MHZ);
        lv_obj_set_style_text_color(headerLabel, lv_palette_main(LV_PALETTE_CYAN), 0);
        lv_label_set_text(statusLabel, "RX mode / BOOT:TX");
    }
}

static void createUi()
{
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), 0);

    lv_obj_t *window = lv_obj_create(lv_scr_act());
    lv_obj_set_size(window, GLASS_VIEWABLE_WIDTH, GLASS_VIEWABLE_HEIGHT);
    lv_obj_align(window, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_scrollbar_mode(window, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(window, lv_color_black(), 0);
    lv_obj_set_style_border_width(window, 0, 0);
    lv_obj_set_style_pad_all(window, 2, 0);

    headerLabel = createLine(window, 0, &lv_font_montserrat_12, lv_palette_main(LV_PALETTE_CYAN));
    lv_obj_set_style_text_align(headerLabel, LV_TEXT_ALIGN_CENTER, 0);

    paramsLabel = createLine(window, 15, &lv_font_montserrat_10, lv_color_white());
    lv_label_set_long_mode(paramsLabel, LV_LABEL_LONG_WRAP);
    lv_label_set_text_fmt(paramsLabel,
                          "BW:%.0f SF:%u CR:4/%u\nSW:%02X P:%d PL:%u",
                          (double)LORA_BANDWIDTH_KHZ,
                          LORA_SPREADING_FACTOR,
                          LORA_CODING_RATE,
                          LORA_SYNC_WORD,
                          LORA_TX_POWER_DBM,
                          LORA_PREAMBLE_LENGTH);

    txLabel = createLine(window, 40, &lv_font_montserrat_10, lv_palette_main(LV_PALETTE_GREEN));
    lv_label_set_text(txLabel, "TX:--");

    rxLabel = createLine(window, 53, &lv_font_montserrat_10, lv_palette_main(LV_PALETTE_YELLOW));
    lv_label_set_text(rxLabel, "RX:--");

    rssiLabel = createLine(window, 66, &lv_font_montserrat_10, lv_color_white());
    lv_label_set_text(rssiLabel, "RSSI:-- dBm");

    snrLabel = createLine(window, 79, &lv_font_montserrat_10, lv_color_white());
    lv_label_set_text(snrLabel, "SNR:-- dB");

    statusLabel = createLine(window, 96, &lv_font_montserrat_10, lv_palette_main(LV_PALETTE_BLUE));
    lv_label_set_long_mode(statusLabel, LV_LABEL_LONG_WRAP);
    updateModeUi();
}

static void showRadioError(const char *operation, int state)
{
    Serial.printf("[SX1262] %s failed, code %d\n", operation, state);
    lv_label_set_text_fmt(statusLabel, "%s error:%d", operation, state);
}

static void startTransmit()
{
    radioOperationDone = false;
    lastTxPayload = String(nodeName) + " #" + String(txCounter++);
    lv_label_set_text_fmt(txLabel, "TX:%s", lastTxPayload.c_str());
    lv_label_set_text(statusLabel, "TX sending...");

    Serial.printf("[SX1262] TX: %s\n", lastTxPayload.c_str());
    int state = radio.startTransmit(lastTxPayload);
    if (state == RADIOLIB_ERR_NONE)
    {
        radioActive = true;
    }
    else
    {
        radioActive = false;
        showRadioError("TX start", state);
        nextActionAt = millis() + LORA_TX_INTERVAL_MS;
    }
}

static void handleTransmitComplete()
{
    int state = radio.finishTransmit();
    radioActive = false;

    if (state == RADIOLIB_ERR_NONE)
    {
        Serial.printf("[SX1262] TX complete: %s\n", lastTxPayload.c_str());
        lv_label_set_text(statusLabel, "TX complete / BOOT:RX");
    }
    else
    {
        showRadioError("TX finish", state);
    }

    nextActionAt = millis() + LORA_TX_INTERVAL_MS;
}

static void startReceive()
{
    radioOperationDone = false;
    int state = radio.startReceive();
    if (state == RADIOLIB_ERR_NONE)
    {
        radioActive = true;
        lv_label_set_text(statusLabel, "RX waiting / BOOT:TX");
        Serial.println("[SX1262] Listening");
    }
    else
    {
        radioActive = false;
        showRadioError("RX start", state);
        nextActionAt = millis() + 1000;
    }
}

static void handleReceiveComplete()
{
    radioActive = false;
    String payload;
    int state = radio.readData(payload);

    if (state == RADIOLIB_ERR_NONE)
    {
        float rssi = radio.getRSSI();
        float snr = radio.getSNR();

        lv_label_set_text_fmt(rxLabel, "RX:%s", payload.c_str());
        lv_label_set_text_fmt(rssiLabel, "RSSI:%.1f dBm", (double)rssi);
        lv_label_set_text_fmt(snrLabel, "SNR:%.1f dB", (double)snr);

        Serial.printf("[SX1262] RX: %s\n", payload.c_str());
        Serial.printf("[SX1262] RSSI: %.1f dBm\n", (double)rssi);
        Serial.printf("[SX1262] SNR: %.1f dB\n", (double)snr);
    }
    else if (state == RADIOLIB_ERR_CRC_MISMATCH)
    {
        Serial.println("[SX1262] CRC mismatch");
        lv_label_set_text(statusLabel, "RX CRC error");
    }
    else
    {
        showRadioError("RX read", state);
    }

    startReceive();
}

static void switchOperatingMode()
{
    int state;
    if (radioActive && operatingMode == MODE_TRANSMIT)
    {
        state = radio.finishTransmit();
    }
    else if (radioActive && operatingMode == MODE_RECEIVE)
    {
        state = radio.finishReceive();
    }
    else
    {
        state = radio.standby();
    }

    if (state != RADIOLIB_ERR_NONE)
    {
        Serial.printf("[Mode] Failed to stop previous mode: %d\n", state);
    }

    radioOperationDone = false;
    radioActive = false;

    if (operatingMode == MODE_RECEIVE)
    {
        operatingMode = MODE_TRANSMIT;
        Serial.println("[Mode] TX");
        updateModeUi();
        nextActionAt = millis();
    }
    else
    {
        operatingMode = MODE_RECEIVE;
        Serial.println("[Mode] RX");
        updateModeUi();
        startReceive();
    }
}

void setup()
{
    Serial.begin(115200);

    if (!glass.begin())
    {
        while (true)
        {
            Serial.println("The board model cannot be detected");
            delay(1000);
        }
    }

    glass.setBrightness(255);
    beginLvglHelper(glass);
    createUi();

    uint16_t nodeId = (uint16_t)(ESP.getEfuseMac() & 0xFFFF);
    snprintf(nodeName, sizeof(nodeName), "TG-%04X", nodeId);

    bootButton.init(BOARD_BOOT_PIN);
    bootButton.setEventCallback(onBootButtonEvent);

    Serial.println();
    Serial.printf("Node: %s\n", nodeName);
    Serial.printf("Frequency: %.1f MHz\n", (double)LORA_FREQUENCY_MHZ);
    Serial.printf("Bandwidth: %.1f kHz\n", (double)LORA_BANDWIDTH_KHZ);
    Serial.printf("SF: %u, CR: 4/%u, Sync: 0x%02X, Power: %d dBm, Preamble: %u\n",
                  LORA_SPREADING_FACTOR,
                  LORA_CODING_RATE,
                  LORA_SYNC_WORD,
                  LORA_TX_POWER_DBM,
                  LORA_PREAMBLE_LENGTH);

    int state = radio.begin(LORA_FREQUENCY_MHZ,
                            LORA_BANDWIDTH_KHZ,
                            LORA_SPREADING_FACTOR,
                            LORA_CODING_RATE,
                            LORA_SYNC_WORD,
                            LORA_TX_POWER_DBM,
                            LORA_PREAMBLE_LENGTH);
    if (state != RADIOLIB_ERR_NONE)
    {
        showRadioError("Radio init", state);
        while (true)
        {
            bootButton.update();
            lv_timer_handler();
            delay(5);
        }
    }

    Serial.println("[SX1262] Initialization successful");
    radio.setDio1Action(onRadioOperationDone);
    startReceive();
}

void loop()
{
    bootButton.update();

    if (modeSwitchRequested)
    {
        modeSwitchRequested = false;
        switchOperatingMode();
    }

    if (radioOperationDone)
    {
        radioOperationDone = false;
        if (operatingMode == MODE_TRANSMIT)
        {
            handleTransmitComplete();
        }
        else
        {
            handleReceiveComplete();
        }
    }

    if (!radioActive && (int32_t)(millis() - nextActionAt) >= 0)
    {
        if (operatingMode == MODE_TRANSMIT)
        {
            startTransmit();
        }
        else
        {
            startReceive();
        }
    }

    glass.update();
    lv_timer_handler();
    delay(2);
}
