#include "PCA9570.h"

PCA9570::PCA9570(uint8_t addr)
{
    _i2cAddr = addr;
}

bool PCA9570::begin()
{
    Wire.begin();
    Wire.beginTransmission(_i2cAddr);
    byte status = Wire.endTransmission();
    return status == 0;
}

void PCA9570::setI2cClock(uint32_t clockHz)
{
    if (clockHz > 1000000) clockHz = 1000000;
    Wire.setClock(clockHz);
}

void PCA9570::writePort(uint8_t value)
{
    Wire.beginTransmission(_i2cAddr);
    Wire.write(value & 0x0F);
    Wire.endTransmission();
}

void PCA9570::digitalWrite(uint8_t pin, uint8_t state)
{
    if (pin > 3) return;

    uint8_t currentVal = readPort();
    if (state == HIGH) {
        currentVal |= (1 << pin);
    } else {
        currentVal &= ~(1 << pin);
    }
    writePort(currentVal);
}

uint8_t PCA9570::readPort()
{
    Wire.requestFrom(_i2cAddr, (uint8_t)1);
    if (Wire.available()) {
        return Wire.read();
    }
    return 0xFF;
}

bool PCA9570::softwareReset()
{
    Wire.beginTransmission(PCA9570_GENERAL_CALL_ADDR);
    Wire.write(PCA9570_SOFT_RESET_CMD);
    byte status = Wire.endTransmission();
    delay(10);
    return status == 0;
}

bool PCA9570::readDeviceID(uint16_t &manufacturerID, uint16_t &partID, uint8_t &revision)
{
    Wire.beginTransmission(PCA9570_DEVICE_ID_WRITE);
    Wire.write(_i2cAddr);
    byte status = Wire.endTransmission(false);
    if (status != 0) return false;

    Wire.requestFrom(PCA9570_DEVICE_ID_READ, (uint8_t)3);
    if (Wire.available() != 3) return false;

    uint8_t idByte1 = Wire.read();
    uint8_t idByte2 = Wire.read();
    uint8_t idByte3 = Wire.read();

    manufacturerID = (idByte1 << 4) | ((idByte2 >> 4) & 0x0F);
    partID = ((idByte2 & 0x0F) << 5) | ((idByte3 >> 3) & 0x1F);
    revision = idByte3 & 0x07;

    return true;
}