<div align="center" markdown="1">
  <img src="../.github/LilyGo_logo.png" alt="LilyGo logo" width="100"/>
</div>

<h1 align = "center">🌟LilyGo T-Glass V3 Factory Firmware🌟</h1>

## Support Product

| Product(PinMap) | Firmware                                                                          |
| --------------- | --------------------------------------------------------------------------------- |
| [T-Glass-V3][1] | [T-Glass-Factory-0.1.0-20260714_0x0](./t-glass-v3-factory-0.1.0-20260714_0x0.bin) |

[1]: https://www.lilygo.cc/products/t-glass

## How to Flash Firmware?

### Preparation - Enter Download Mode

1. Connect the board via USB cable
2. Press and hold the **BOOT** button
3. Press the **RST** button
4. Release the **RST** button
5. Release the **BOOT** button
6. Upload firmware
7. Press the **RST** button to exit download mode

### Method 1: LILYGO Spark

1. Download [LILYGO Spark](https://lilygo.cc/pages/lilygo-spark)

![](./images/LILYGO-Spark.png)

### Method 2: Using ESP Download Tool

1. Download [Flash_download_tool](https://docs.espressif.com/projects/esp-test-tools/en/latest/esp32/production_stage/tools/flash_download_tool.html)

> **Note**: The following GIF demonstrates the flashing process for ESP32-S3. If you are using ESP32, select ESP32 instead of ESP32-S3.

![esp download Tool](./images/esp_downloader.gif)

**After flashing is complete, press the RST button to reset**

### Method 3: Using Web Flasher

- [ESP Web Flasher Online Tool](https://espressif.github.io/esptool-js/)

![Web Flasher](./images/web_flasher.gif)

**After flashing is complete, press the RST button to reset**

### Method 4: Using Command Line

If the system prompts to install development tools, please install them first:

```bash
python3 -m pip install --upgrade pip
python3 -m pip install esptool
```

Start esptool.py:

```bash
python3 -m esptool
```

#### ESP32 Flashing Command

```bash
esptool --chip esp32 --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m 0x0 firmware.bin
```

#### ESP32-S3 Flashing Command

```bash
esptool --chip esp32s3 --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m 0x0 firmware.bin
```

## Related Resources

- [Espressif Official Documentation](https://docs.espressif.com/projects/esptool/)
