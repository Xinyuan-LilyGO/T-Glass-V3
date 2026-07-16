/**
 * @file      LilyGo_GlassV3.h
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2026  ShenZhen XinYuan Electronic Technology Co., Ltd
 * @date      2026-07-16
 *
 */
#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <GaugeBQ27220.hpp>
#include <PowersBQ25896.tpp>
#include "LilyGo_Display.h"
#include "LilyGo_Button.h"
#include <driver/i2s.h>
#include <RadioLib.h>
#include <esp_camera.h>

/* ── Build-time sanity checks ──────────────────────────────────────────── */

#if ARDUINO_USB_CDC_ON_BOOT != 1
#warning "If you need to monitor printed data, be sure to set USB_CDC_ON_BOOT to ENABLE, otherwise you will not see any data in the serial monitor"
#endif

#ifndef BOARD_HAS_PSRAM
#error "Detected that PSRAM is not turned on. Please set PSRAM to QSPI PSRAM in ArduinoIDE"
#endif //BOARD_HAS_PSRAM


#ifndef  SW_ROTATION
#define SW_ROTATION
#endif

/* ── Display SPI pin definitions ───────────────────────────────────────── */

#define BOARD_NONE_PIN              (-1)
#define BOARD_DISP_CS               (37)    /**< Display chip-select            */
#define BOARD_DISP_SCK              (36)    /**< Display SPI clock              */
#define BOARD_DISP_MISO             (34)    /**< Display SPI MISO               */
#define BOARD_DISP_MOSI             (35)    /**< Display SPI MOSI               */
#define BOARD_DISP_DC               (46)    /**< Display data/command           */
#define BOARD_DISP_RST              (38)    /**< Display hardware reset         */

/* ── I2C bus ────────────────────────────────────────────────────────────── */

#define BOARD_I2C_SDA               (3)
#define BOARD_I2C_SCL               (2)

/* ── Button / Boot ─────────────────────────────────────────────────────── */

#define BOARD_TOUCH_BUTTON          (1)
#define BOARD_BOOT_PIN              (0)

/* ── SPI clock speed ───────────────────────────────────────────────────── */

#define DEFAULT_SCK_SPEED           (40 * 1000 * 1000)

/* ── Camera (ESP32-S3 camera interface) ─────────────────────────────────── */

#define PWDN_GPIO_NUM               (-1)
#define RESET_GPIO_NUM              (8)
#define XCLK_GPIO_NUM               (4)
#define SIOD_GPIO_NUM               (10)    /**< Camera SDA (SCCB)             */
#define SIOC_GPIO_NUM               (9)     /**< Camera SCL (SCCB)             */
#define VSYNC_GPIO_NUM              (7)
#define HREF_GPIO_NUM               (6)
#define PCLK_GPIO_NUM               (17)
#define Y9_GPIO_NUM                 (5)
#define Y8_GPIO_NUM                 (16)
#define Y7_GPIO_NUM                 (15)
#define Y6_GPIO_NUM                 (18)
#define Y5_GPIO_NUM                 (13)
#define Y4_GPIO_NUM                 (11)
#define Y3_GPIO_NUM                 (12)
#define Y2_GPIO_NUM                 (14)

/* ── Audio codec (ES8311) and microphone (ES7210) I2S pins ──────────────── */

#define I2S_WS                      (42)    /**< LRCK / Word-select            */
#define I2S_SCK                     (40)    /**< SCLK / Bit-clock              */
#define I2S_MCLK                    (39)    /**< MCLK  / Master clock          */
#define I2S_SDOUT                   (45)    /**< DSDIN / DAC serial data in     */
#define I2S_SDIN                    (41)    /**< ASDOUT/ ADC serial data out    */

/* ── LoRa radio (SX1262) — shares SPI bus with display ─────────────────── */

#define LORA_SCK                    (36)    /**< Shared SPI clock              */
#define LORA_MISO                   (34)    /**< Shared SPI MISO               */
#define LORA_MOSI                   (35)    /**< Shared SPI MOSI               */
#define LORA_CS                     (33)    /**< LoRa chip-select              */
#define LORA_RST                    (47)    /**< LoRa hardware reset           */
#define LORA_BUSY                   (48)    /**< LoRa busy / DIO1              */
#define LORA_IRQ                    (21)    /**< LoRa interrupt                 */

/* ── LCD command constant ───────────────────────────────────────────────── */

#define  LCD_CMD_RGB 0x00

/* ── Audio library includes ─────────────────────────────────────────────── */

#include <AudioFileSourcePROGMEM.h>
#include <AudioGeneratorRTTTL.h>
#include <AudioOutputI2S.h>
#include <ESP8266SAM.h>

#ifdef USE_ESP_CODEC_LIB
#include "bsp_codec/esp_codec.h"
#else
#include "AudioBoard.h"
#endif

/* ── Microphone I2S port ───────────────────────────────────────────────── */

#ifndef MIC_I2S_PORT
#define MIC_I2S_PORT    I2S_NUM_1
#endif


class LilyGo_Glass : public LilyGo_Display
#ifdef USE_BUILTIN_TOUCH
    , public LilyGo_Button
#endif
{
public:

    /* ── Peripheral instances ────────────────────────────────────────────── */

    GaugeBQ27220 gauge;              /**< Battery fuel-gauge driver          */
    PowersBQ25896 ppm;               /**< Power-path / charger driver        */

    AudioOutputI2S *audioOut = nullptr;       /**< I2S audio output stream    */
    AudioGeneratorRTTTL *i2sRtttl = nullptr;  /**< RTTTL tone generator       */
    AudioFileSourcePROGMEM *rtttlFile = nullptr; /**< PROGMEM audio source     */

    /* ── Lifecycle ───────────────────────────────────────────────────────── */

    LilyGo_Glass();
    ~LilyGo_Glass();

    /**
     * @brief  Initialise all onboard peripherals (I2C, display, gauge, etc.).
     * @return true on success, false if a critical peripheral fails to respond.
     */
    bool begin();

    /**
     * @brief  Initialise the ESP32-S3 camera interface.
     * @return true on success.
     */
    bool initCamera();

    /* ── I2S audio / microphone ──────────────────────────────────────────── */

    /**
     * @brief  Open the I2S bus for audio input/output.
     * @return true on success.
     */
    bool initI2S();

    /** @brief  Release the I2S bus and associated DMA buffers. */
    void deinitI2S();

    /**
     * @brief  Read current left/right audio input levels (0 – 16384).
     * @param[out] leftLevel   Left-channel peak value.
     * @param[out] rightLevel  Right-channel peak value.
     */
    void getAudioLevels(int *leftLevel, int *rightLevel);

    /* ── Periodic maintenance ────────────────────────────────────────────── */

    /** @brief  Poll gauge & charger registers; call in loop(). */
    void update();

    /* ── Display control ─────────────────────────────────────────────────── */

    /**
     * @brief  Set display backlight brightness.
     * @param level  Brightness value (0 = off, 255 = full).
     */
    void setBrightness(uint8_t level);

    /**
     * @brief  Get current brightness level.
     * @return Brightness value (0 – 255).
     */
    uint8_t getBrightness();

    /**
     * @brief  Set the display rotation.
     * @param rotation  0 = 0°, 1 = 90°, 2 = 180°, 3 = 270°.
     */
    void setRotation(uint8_t rotation);

    /**
     * @brief  Get the current rotation index.
     * @return Rotation value (0 – 3).
     */
    uint8_t getRotation();

    /**
     * @brief  Mirror the display horizontally.
     * @param enable  true to flip, false to restore normal orientation.
     */
    void flipHorizontal(bool enable);

    /* ── Framebuffer operations ──────────────────────────────────────────── */

    /**
     * @brief  Set the hardware drawing window (bypasses software rotation).
     * @param xs  Start X (column).
     * @param ys  Start Y (row).
     * @param xe  End X.
     * @param ye  End Y.
     */
    void setAddrWindow(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye);

    /**
     * @brief  Write a contiguous pixel buffer to the current window (no rotation).
     * @param data  Pointer to 16-bit RGB565 pixel data.
     * @param len   Number of pixels.
     */
    void pushColors(uint16_t *data, uint32_t len);

    /**
     * @brief  Write a rectangular region with software rotation support.
     * @param x       Top-left X.
     * @param y       Top-left Y.
     * @param width   Region width in pixels.
     * @param height  Region height in pixels.
     * @param data    Pointer to 16-bit RGB565 pixel data.
     */
    void pushColors(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t *data);

    /**
     * @brief  Return the active display width (accounting for rotation).
     * @return Width in pixels.
     */
    uint16_t  width();

    /**
     * @brief  Return the active display height (accounting for rotation).
     * @return Height in pixels.
     */
    uint16_t  height();

    /**
     * @brief  Check whether a touch panel is connected.
     * @return true if touch hardware is detected.
     */
    bool hasTouch();

    /**
     * @brief  Read the current touch coordinates.
     * @param[out] x         Touch X coordinate.
     * @param[out] y         Touch Y coordinate.
     * @param      get_point Number of touch points to read.
     * @return Number of valid touch points (0 = no touch).
     */
    uint8_t getPoint(int16_t *x, int16_t *y, uint8_t get_point );

    /* ── Battery ─────────────────────────────────────────────────────────── */

    /**
     * @brief  Read the battery voltage from the fuel gauge.
     * @return Voltage in millivolts.
     */
    uint16_t getBattVoltage();

    /**
     * @brief  Get the estimated battery state-of-charge.
     * @return Percentage (0 – 100), or -1 on error.
     */
    int getBatteryPercent();

    /* ── Power management ────────────────────────────────────────────────── */

    /**
     * @brief  Configure touch as a deep-sleep wakeup source.
     * @param threshold  Touch threshold for wakeup (default 2000).
     */
    void enableTouchWakeup(uint32_t threshold = 2000);

    /** @brief  Enter deep-sleep until a wakeup source triggers. */
    void sleep();

    /** @brief  Wake from deep-sleep (re-initialise peripherals). */
    void wakeup();

    /**
     * @brief  Check whether a full display refresh is needed after wakeup.
     * @return true if the frame buffer must be re-drawn.
     */
    bool needFullRefresh();

    /* ── Microphone streaming ────────────────────────────────────────────── */

    /**
     * @brief  Initialise the ES7210 microphone array over I2S.
     * @return true on success.
     */
    bool initMicrophone();

    /**
     * @brief  Read audio samples from the microphone I2S stream.
     * @param[out] dest          Destination buffer.
     * @param      size          Number of bytes to read.
     * @param[out] bytes_read    Actual bytes read.
     * @param      ticks_to_wait Maximum ticks to block (default: forever).
     * @return true on success.
     */
    bool readStream(void *dest, size_t size, size_t *bytes_read, TickType_t ticks_to_wait = portMAX_DELAY);

    /**
     * @brief  Write audio samples to the speaker I2S stream.
     * @param[in]  src            Source buffer.
     * @param      size           Number of bytes to write.
     * @param[out] bytes_written  Actual bytes written.
     * @param      ticks_to_wait  Maximum ticks to block (default: forever).
     * @return true on success.
     */
    bool writeStream(const void *src, size_t size, size_t *bytes_written, TickType_t ticks_to_wait = portMAX_DELAY);

    /** @brief  Play a short confirmation beep via the audio codec. */
    void tone();

    /**
     * @brief  Check whether the camera module was detected during init.
     * @return true if the camera is available.
     */
    bool isCameraDetected()
    {
        return _camera_detected;
    }


#ifdef USE_BUILTIN_TOUCH
    /* ── Touch input ─────────────────────────────────────────────────────── */

    /**
     * @brief  Set the touch detection threshold.
     * @param threshold  Raw ADC threshold (higher = less sensitive).
     */
    void setTouchThreshold(uint32_t threshold);

    /** @brief  Detach touch interrupt and stop polling. */
    void detachTouch();

    /**
     * @brief  Check whether the button is currently being touched.
     * @return true if touched.
     */
    bool getTouched();

    /**
     * @brief  Edge-triggered press detection (returns true once per press).
     * @return true on a new press event.
     */
    bool isPressed();
#endif

private:
    /** @brief  Initialise shared SPI / I2C buses. */
    bool initBUS();

    /**
     * @brief  Send a raw command to the display controller.
     * @param cmd     Command code.
     * @param pdat    Optional parameter data.
     * @param length  Length of parameter data in bytes.
     */
    void writeCommand(uint32_t cmd, uint8_t *pdat, uint32_t length);

    uint8_t _brightness;            /**< Current backlight brightness         */
    uint32_t  threshold ;           /**< Touch detection threshold            */
    uint8_t _rotation;              /**< Current rotation index (0-3)         */
    bool _flipHorizontal;           /**< Horizontal mirror flag               */
    uint16_t _width;                /**< Active display width in pixels       */
    uint16_t _height;               /**< Active display height in pixels      */
    uint16_t *_frame_buffer;        /**< Pointer to the software frame buffer */
    bool _gauge_online;             /**< true if BQ27220 was found on I2C     */
    bool _es8311_detected;          /**< true if ES8311 codec was found       */
    bool _es7210_detected;          /**< true if ES7210 mic array was found   */
    bool _camera_detected;          /**< true if camera module was detected   */
    uint32_t _last_ref_data;        /**< Timestamp of last gauge reference    */
    uint32_t _ref_data_interval = 3000; /**< Gauge reference interval (ms)   */
};

/** @brief Audio output device instance (codec DAC). */
extern AudioBoard audioOutputDev;

/** @brief Audio input device instance (microphone). */
extern AudioBoard audioInputDev;

/** @brief Global LilyGo_Glass instance for application use. */
extern LilyGo_Glass glass;
