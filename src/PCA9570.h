/**
 * @file      PCA9570.h
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2026  ShenZhen XinYuan Electronic Technology Co., Ltd
 * @date      2026-07-16
 *
 * @brief     Driver for the TI PCA9570 4-bit I2C GPIO expander.
 */
#ifndef PCA9570_h
#define PCA9570_h

#include <Arduino.h>
#include <Wire.h>

/* ── I2C address constants ─────────────────────────────────────────────── */

#define PCA9570_DEFAULT_ADDR        0x24    /**< Default 7-bit I2C address     */
#define PCA9570_GENERAL_CALL_ADDR   0x00    /**< I2C general-call address      */
#define PCA9570_DEVICE_ID_WRITE     0xF8    /**< Device-ID write command       */
#define PCA9570_DEVICE_ID_READ      0xF9    /**< Device-ID read command        */
#define PCA9570_SOFT_RESET_CMD      0x06    /**< Software-reset command byte   */

/**
 * @class PCA9570
 * @brief I2C 4-bit GPIO expander driver.
 *
 * Provides individual pin control and bulk read/write on the PCA9570's
 * 4 GPIO ports (P0-P3).  All pins default to open-drain outputs.
 */
class PCA9570
{
private:
    uint8_t _i2cAddr;               /**< Current I2C slave address          */

public:
    /**
     * @brief  Construct a PCA9570 driver instance.
     * @param addr  7-bit I2C address (default 0x24).
     */
    PCA9570(uint8_t addr = PCA9570_DEFAULT_ADDR);

    /**
     * @brief  Initialise I2C and verify the device is present.
     * @return true if the device ACKed on the bus.
     */
    bool begin();

    /**
     * @brief  Set a single GPIO pin HIGH or LOW.
     * @param pin    Pin index (0-3).
     * @param state  HIGH or LOW.
     */
    void digitalWrite(uint8_t pin, uint8_t state);

    /**
     * @brief  Write all 4 pins at once (lower nibble of @p value).
     * @param value  Bitmask for P3..P0 (bits 3-0).
     */
    void writePort(uint8_t value);

    /**
     * @brief  Read the current state of all 4 pins.
     * @return 4-bit value (bits 3-0 = P3..P0), or 0xFF on read failure.
     */
    uint8_t readPort();

    /**
     * @brief  Issue a general-call software reset.
     * @return true if the reset command was ACKed.
     */
    bool softwareReset();

    /**
     * @brief  Read the device identification register.
     * @param[out] manufacturerID  12-bit manufacturer ID.
     * @param[out] partID          13-bit part number.
     * @param[out] revision        3-bit revision field.
     * @return true on success, false on I2C error.
     */
    bool readDeviceID(uint16_t &manufacturerID, uint16_t &partID, uint8_t &revision);

    /**
     * @brief  Set the I2C bus clock frequency.
     * @param clockHz  Desired clock in Hz (capped at 1 MHz).
     */
    void setI2cClock(uint32_t clockHz);
};

#endif
