/*
  RadioLib SX126x Transmit with Interrupts Example

  This example transmits LoRa packets with one second delays
  between them. Each packet contains up to 256 bytes
  of data, in the form of:
  - Arduino String
  - null-terminated char array (C-string)
  - arbitrary binary data (byte array)

  Other modules from SX126x family can also be used.

  For default module settings, see the wiki page
  https://github.com/jgromes/RadioLib/wiki/Default-configuration#sx126x---lora-modem

  For full API reference, see the GitHub Pages
  https://jgromes.github.io/RadioLib/
*/

// include the library
#include <LilyGo_GlassV3.h>
#include <LV_Helper.h>

// The resolution of the non-magnified side of the glasses reflection area is about 126x126,
// and the magnified area is smaller than 126x126
#define GlassViewableWidth              126
#define GlassViewableHeight             126

// save transmission state between loops
int transmissionState = RADIOLIB_ERR_NONE;

SX1262 radio = new Module(LORA_CS, LORA_IRQ, LORA_RST, LORA_BUSY);

// flag to indicate that a packet was sent
volatile bool transmittedFlag = false;
lv_obj_t *label;

void setFlag(void)
{
    // we sent a packet, set the flag
    transmittedFlag = true;
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
    label = lv_label_create(window);
    lv_obj_set_style_text_color(label, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_22, LV_PART_MAIN);
    lv_label_set_text(label, "LoRa Transmitter");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    // freq – Carrier frequency in MHz. Defaults to 434.0 MHz.
    // bw – LoRa bandwidth in kHz. Defaults to 125.0 kHz.
    // sf – LoRa spreading factor. Defaults to 9.
    // cr – LoRa coding rate denominator. Defaults to 7 (coding rate 4/7). Allowed values range from 4 to 8. Note that a value of 4 means no coding, is undocumented and not recommended without your own FEC.
    // syncWord – 1-byte LoRa sync word. Defaults to RADIOLIB_SX126X_SYNC_WORD_PRIVATE (0x12).
    // power – Output power in dBm. Defaults to 10 dBm.
    // preambleLength – LoRa preamble length in symbols. Defaults to 8 symbols.

    float freq = 434.0;
    float bw = 125.0;
    uint8_t sf = 9;
    uint8_t cr = 7;
    uint8_t syncWord = 0x12;
    int8_t power = 10;
    uint16_t preambleLength = 8;

    int state = radio.begin(freq, bw, sf, cr, syncWord, power, preambleLength);
    if (state == RADIOLIB_ERR_NONE) {
        Serial.printf("Radio initialized successfully\n");
    } else {
        Serial.printf("Radio initialization failed\n");
        lv_label_set_text_fmt(label, "Failed to initialize radio!");
        while (true) {
            delay(10);
            lv_timer_handler();
        }
    }

    // set the function that will be called
    // when packet transmission is finished
    radio.setPacketSentAction(setFlag);

    // start transmitting the first packet
    Serial.print(F("[SX1262] Sending first packet ... "));

    // you can transmit C-string or Arduino string up to
    // 256 characters long
    transmissionState = radio.startTransmit("Hello World!");

    // you can also transmit byte array up to 256 bytes long
    /*
      byte byteArr[] = {0x01, 0x23, 0x45, 0x67,
                        0x89, 0xAB, 0xCD, 0xEF};
      state = radio.startTransmit(byteArr, 8);
    */
}

// counter to keep track of transmitted packets
int count = 0;

void loop()
{
    // check if the previous transmission finished
    if (transmittedFlag) {
        // reset flag
        transmittedFlag = false;

        if (transmissionState == RADIOLIB_ERR_NONE) {
            // packet was successfully sent
            Serial.println(F("transmission finished!"));

            // NOTE: when using interrupt-driven transmit method,
            //       it is not possible to automatically measure
            //       transmission data rate using getDataRate()
            lv_label_set_text_fmt(label, "Packet #%d sent!", count);

        } else {
            Serial.print(F("failed, code "));
            Serial.println(transmissionState);
            lv_label_set_text_fmt(label, "Packet #%d failed!", count);

        }

        // clean up after transmission is finished
        // this will ensure transmitter is disabled,
        // RF switch is powered down etc.
        radio.finishTransmit();

        // wait a second before transmitting again
        delay(1000);

        // send another one
        Serial.print(F("[SX1262] Sending another packet ... "));

        // you can transmit C-string or Arduino string up to
        // 256 characters long
        String str = "Hello World! #" + String(count++);
        transmissionState = radio.startTransmit(str);

        // you can also transmit byte array up to 256 bytes long
        /*
          byte byteArr[] = {0x01, 0x23, 0x45, 0x67,
                            0x89, 0xAB, 0xCD, 0xEF};
          transmissionState = radio.startTransmit(byteArr, 8);
        */
    }

    lv_timer_handler();

}
