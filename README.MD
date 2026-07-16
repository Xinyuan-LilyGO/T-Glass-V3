<div align="center" markdown="1">
  <img src=".github/LilyGo_logo.png" alt="LilyGo logo" width="100"/>
</div>

<h1 align = "center">🌟T-GlassV3🌟</h1>

## Product

| Name                 | Params         |
| -------------------- | -------------- |
| SOC                  | ESP32-S3 FN4R2 |
| Flash                | 4MB            |
| PSRAM                | 2MB(QSPI)      |
| Display Resolution   | 294x126        |
| Display Visible Area | 126x126        |
| Camera               | OV2640         |

## Examples

```txt
examples/
├── BatteryVoltage
├── CameraWebServer
├── DeepSleep
├── Factory
├── HelloWorld
├── RecordWAV
├── SX126x_PingPong
├── SX126x_Receive_Interrupt
├── SX126x_Transmit_Interrupt
├── SimplePlayback
├── SimplePlaybackWithTouch
└── TouchButton
```

## PlatformIO Quick Start (Recommended)

1. Install [Visual Studio Code](https://code.visualstudio.com/) and [Python](https://www.python.org/)
2. Search for the `PlatformIO` plugin in the `VisualStudioCode` extension and install it.
3. After the installation is complete, you need to restart `VisualStudioCode`
4. After restarting `VisualStudioCode`, select `File` in the upper left corner of `VisualStudioCode` -> `Open Folder` -> select the `T-GlassV3` directory
5. Wait for the installation of third-party dependent libraries to complete
6. Click on the `platformio.ini` file, and in the `platformio` column
7. Uncomment one of the lines `src_dir = xxxx` to make sure only one line works
8. Click the (✔) symbol in the lower left corner to compile
9. Connect the board to the computer USB
10. Click (→) to upload firmware
11. Click (plug symbol) to monitor serial output
12. If it cannot be written, or the USB device keeps flashing, please check the **FAQ** below

## Arduino IDE Manual installation (not recommended)

1. Install [Arduino IDE](https://www.arduino.cc/en/software)
2. Install [Arduino ESP32 V 2.0.5 or above and below V3.0](https://docs.espressif.com/projects/arduino-esp32/en/latest/getting_started.html), Library based on the Stable version, Please make sure the link in Boards Manager is

   ```url
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```

3. Download `T-GlassV3` , move to Arduino library folder (e.g. C:\Users\YourName\Documents\Arduino\libraries)
4. Copy all folders in [libdeps folder](./libdeps/) to Arduino library folder (e.g. C:\Users\YourName\Documents\Arduino\libraries)
5. `Tools` -> , Make your selection according to the table below

   | Arduino IDE Setting                  | Value                                             |
   | ------------------------------------ | ------------------------------------------------- |
   | Board                                | **ESP32S3 Dev Module**                            |
   | Port                                 | Your port                                         |
   | USB CDC On Boot                      | Enable                                            |
   | CPU Frequency                        | 240MHZ(WiFi)                                      |
   | Core Debug Level                     | None                                              |
   | USB DFU On Boot                      | Disable                                           |
   | Erase All Flash Before Sketch Upload | Disable                                           |
   | Events Run On                        | Core1                                             |
   | Flash Mode                           | QIO 80MHZ                                         |
   | Flash Size                           | **4MB(32Mb)**                                     |
   | Arduino Runs On                      | Core1                                             |
   | USB Firmware MSC On Boot             | Disable                                           |
   | Partition Scheme                     | **Default 4M with spiffs(1.2M APP/1.5MB SPIFFS)** |
   | PSRAM                                | **QSPI PSRAM**                                    |
   | Upload Mode                          | **UART0/Hardware CDC**                            |
   | Upload Speed                         | 921600                                            |
   | USB Mode                             | **CDC and JTAG**                                  |

   - The options in bold are required, others are selected according to actual conditions.

6. `File` -> `Examples` -> `T-GlassV3` -> `HelloWorld`
7. Select `Port`
8. Click `upload` , Wait for compilation and writing to complete
9.  If it cannot be written, or the USB device keeps flashing, please check the **FAQ** below


## T-GlassV3 Pinout

#### Display (SPI)

| Name        | GPIO | Note |
| ----------- | ---- | ---- |
| MOSI        | 35   |      |
| MISO        | 34   |      |
| SCLK        | 36   |      |
| DC          | 46   |      |
| Display CS  | 37   |      |
| Display RST | 38   |      |

#### LoRa

| Name      | GPIO | Note                        |
| --------- | ---- | --------------------------- |
| LoRa CS   | 33   | Shares SPI bus with Display |
| LoRa RST  | 47   | Shares SPI bus with Display |
| LoRa BUSY | 48   | Shares SPI bus with Display |
| LoRa IRQ  | 21   | Shares SPI bus with Display |

#### I2C

| Name | GPIO | Note |
| ---- | ---- | ---- |
| SDA  | 3    |      |
| SCL  | 2    |      |

#### Camera

| Name  | GPIO | Note |
| ----- | ---- | ---- |
| PWDN  | -1   |      |
| RESET | 8    |      |
| XCLK  | 4    |      |
| SIOD  | 10   |      |
| SIOC  | 9    |      |
| VSYNC | 7    |      |
| HREF  | 6    |      |
| PCLK  | 17   |      |
| Y9    | 5    |      |
| Y8    | 16   |      |
| Y7    | 15   |      |
| Y6    | 18   |      |
| Y5    | 13   |      |
| Y4    | 11   |      |
| Y3    | 12   |      |
| Y2    | 14   |      |

#### I2S (Audio)

| Name  | GPIO | Note   |
| ----- | ---- | ------ |
| WS    | 42   | LRCK   |
| SCK   | 40   | SCLK   |
| MCLK  | 39   |        |
| SDOUT | 45   | DSDIN  |
| SDIN  | 41   | ASDOUT |

#### Button

| Button | GPIO | Note                                                 |
| ------ | ---- | ---------------------------------------------------- |
| RESET  | N.A  | Reset Device                                         |
| PWR    | N.A  | Exit Shipping Mode Function Button                   |
| BOOT   | 0    | Customizable buttons, download mode function buttons |
| Touch  | 1    | Touch button                                         |

### 🧑🏼‍🔧 I2C Devices Address

| Device        | 7-Bit Address |             | Note           |
| ------------- | ------------- | ----------- | -------------- |
| [ES8311][2]   | 0x18          | Audio Codec | Shared I2C Bus |
| [ES7210][3]   | 0x40          | Audio ADC   | Shared I2C Bus |
| [BQ25896][4]  | 0x6B          | Charger     | Shared I2C Bus |
| [BQ27220][5]  | 0x55          | Gauge       | Shared I2C Bus |
| [SGM38121][6] | 0x28          | Camera Pmic | Shared I2C Bus |

### ⚡ Electrical Parameters

| Feature             | Details       |
| ------------------- | ------------- |
| USB-C Input Voltage | 4.8V - 5.5V   |
| Charge Current      | 200 mA        |
| Battery Voltage     | 3.7V          |
| Battery Capacity    | 500 mA        |
| Battery Connector   | [MX 1.2mm][7] |
| Charge Temperature  | 0°C ~ 60°C    |

> [!IMPORTANT]
>
> The BQ25896 charging chip has a default charging current of 2A. This library has already set the charging chip to a charging current of 200mA during initialization. If you are not using this library, please set a lower charging current for the BQ25896 during initialization, preferably half the battery capacity
>

- Double-click the RESET button to enter DFU mode.
- Single-click the RESET button to reset the device.

## ESP32 basic examples

- [BLE Examples](https://github.com/espressif/arduino-esp32/tree/master/libraries/BLE)
- [WiFi Examples](https://github.com/espressif/arduino-esp32/tree/master/libraries/WiFi)
- [SPIFFS Examples](https://github.com/espressif/arduino-esp32/tree/master/libraries/SPIFFS)
- [FFat Examples](https://github.com/espressif/arduino-esp32/tree/master/libraries/FFat)
- For more examples of esp32 chip functions, please refer to [arduino-esp32-libraries](https://github.com/espressif/arduino-esp32/tree/master/libraries)

## FAQ

1. The board uses USB as the **JTAG** upload port. When printing serial port information on **USB_CDC_ON_BOOT** configuration needs to be turned on.
   If the port cannot be found when uploading the program or the USB has been used for other functions, the port does not appear.
   Please enter the upload mode manually.

   1. Connect the board via the USB cable
   2. Press and hold the **BOOT button** , While still pressing the **BOOT button**, press **RST button**
   3. Release the **RST button**
   4. Release the **BOOT button**
   6. Upload sketch
   7. Press the **RST** button to exit download mode

2. If the above is invalid, burn the [binary file](./firmware/README.MD) to check whether the hardware is normal
3. If you use external power supply instead of USB-C, please turn off the **USB_CDC_ON_BOOT** option. This is because the board will wait for USB access when it starts.

   - For Arduino IDE users, it can be turned off in the options , Please note that turning off **USB_CDC_ON_BOOT** will turn off Serial redirection to USB-C. At this time, you will not see any Serial message output when opening the port from USB-C, but output from GPIO43 and GPIO44.

   ```c
   Tools -> USB CDC On Boot -> Disable
   ```

   - For Platformio users, you can add the following compilation flags in the ini file

   ```c
   build_flags =
       ; Enable -DARDUINO_USB_CDC_ON_BOOT will start printing and wait for terminal access during startup
       ; -DARDUINO_USB_CDC_ON_BOOT=1

       ; Enable -UARDUINO_USB_CDC_ON_BOOT will turn off printing and will not block when using the battery
       -UARDUINO_USB_CDC_ON_BOOT
   ```

# Resource

- [T-Glass-V3 schematic](./schematic/T-Glass-V3.pdf)
- [JD9613 Display](https://github.com/Xinyuan-LilyGO/T-Track/blob/main/datasheet/JD9613_DS_Preliminary_V0.02_20210325.pdf)
- [ESP32S3-FN4R2](https://www.espressif.com.cn/en/support/documents/technical-documents)
- [BHI260AP](https://www.bosch-sensortec.com/products/smart-sensor-systems/bhi260ap/)

# Depends on required libraries

- [SensorLib 0.3.3](https://github.com/lewisxhe/SensorsLib)
- [lvgl 8.4.0](https://github.com/lvgl/lvgl/tree/release/v8.4)
- [XPowersLib 0.3.2](https://github.com/lewisxhe/XPowersLib)
- [RadioLib 7.7.1](https://github.com/jgromes/RadioLib)
- [ESP8266Audio 1.9.9](https://github.com/earlephilhower/ESP8266Audio)
- [ESP8266SAM 1.1.0](https://github.com/earlephilhower/ESP8266SAM)
- [arduino-audio-driver 0.2.0](https://github.com/pschatzmann/arduino-audio-driver)


[1]: https://www.lilygo.cc/products/t-glass
[2]: http://www.everest-semi.com/pdf/ES8311%20PB.pdf
[3]: http://www.everest-semi.com/pdf/ES7210%20PB.pdf
[4]: https://www.ti.com/product/BQ25896
[5]: https://www.ti.com/product/BQ27220
[6]: https://www.sg-micro.com/rect/assets/6aadf126-e29c-4b05-9225-2b7969ae2dd1/SGM38121.pdf
[7]: https://www.molex.com/content/dam/molex/molex-dot-com/products/automated/en-us/salesdrawingpdf/781/78171/781710003_sd.pdf?inline\
