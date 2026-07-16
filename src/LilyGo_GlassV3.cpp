/**
 * @file      LilyGo_Glass.cpp
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2026  ShenZhen XinYuan Electronic Technology Co., Ltd
 * @date      2026-07-16
 *
 */
#include <esp_lcd_panel_commands.h>
#include "LilyGo_GlassV3.h"
#include "initSequence.h"
#include "PCA9570.h"

static constexpr uint8_t _ES8311_ADDRESS = 0x18;
static constexpr uint8_t _ES7210_ADDRESS = 0x40;


#ifdef USE_ESP_CODEC_LIB
EspCodec        codec;
#else
DriverPins PinsAudioBoard;
AudioBoard audioOutputDev(AudioDriverES8311, PinsAudioBoard);
AudioBoard audioInputDev(AudioDriverES7210, PinsAudioBoard);
#endif

static uint32_t _spi_freq = 80000000;

static  camera_config_t _camera_config = {
    .pin_pwdn       = PWDN_GPIO_NUM,
    .pin_reset      = RESET_GPIO_NUM,
    .pin_xclk       = XCLK_GPIO_NUM,
    .pin_sccb_sda      = SIOD_GPIO_NUM,
    .pin_sccb_scl      = SIOC_GPIO_NUM,
    .pin_d7        = Y9_GPIO_NUM,
    .pin_d6        = Y8_GPIO_NUM,
    .pin_d5        = Y7_GPIO_NUM,
    .pin_d4        = Y6_GPIO_NUM,
    .pin_d3        = Y5_GPIO_NUM,
    .pin_d2        = Y4_GPIO_NUM,
    .pin_d1        = Y3_GPIO_NUM,
    .pin_d0        = Y2_GPIO_NUM,
    .pin_vsync     = VSYNC_GPIO_NUM,
    .pin_href      = HREF_GPIO_NUM,
    .pin_pclk      = PCLK_GPIO_NUM,
    .xclk_freq_hz = 20000000,
    .ledc_timer     = LEDC_TIMER_0,
    .ledc_channel   = LEDC_CHANNEL_0,
    .pixel_format   = PIXFORMAT_RGB565, // PIXFORMAT_JPEG,//
    // .pixel_format   = PIXFORMAT_JPEG,//
    .frame_size = FRAMESIZE_96X96,//FRAMESIZE_QVGA,
    .jpeg_quality = 10,
    .fb_count = 2,
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_LATEST,
    .sccb_i2c_port = 1
};

static const char rudolph[] PROGMEM = "PowerOn: d=4,o=5,b=120: c5, e5, 2g5";
static const char tone_rtttl[] PROGMEM = "MenuSwitch: d=8,o=5,b=180: c5, e5";
static PCA9570 expander;
static volatile bool touchDetected;

#ifdef USE_BUILTIN_TOUCH
static void touchISR()
{
    touchDetected = true;
}

static bool touchPadReadFunction()
{
    return touchInterruptGetLastStatus(BOARD_TOUCH_BUTTON) == 0;
}

void LilyGo_Glass::setTouchThreshold(uint32_t threshold)
{
    this->threshold = threshold;
    touchDetachInterrupt(BOARD_TOUCH_BUTTON);
    touchAttachInterrupt(BOARD_TOUCH_BUTTON, touchISR, threshold);
}

void LilyGo_Glass::detachTouch()
{
    touchDetachInterrupt(BOARD_TOUCH_BUTTON);
}

bool LilyGo_Glass::getTouched()
{
    if (touchDetected) {
        touchDetected = false;
        return touchInterruptGetLastStatus(BOARD_TOUCH_BUTTON);
    }
}

bool LilyGo_Glass::isPressed()
{
    return touchInterruptGetLastStatus(BOARD_TOUCH_BUTTON);
}

#endif

LilyGo_Glass::LilyGo_Glass(): _brightness(AMOLED_DEFAULT_BRIGHTNESS),  threshold(2000)
{

}

LilyGo_Glass::~LilyGo_Glass()
{
#ifdef USE_BUILTIN_TOUCH
    touchDetachInterrupt(BOARD_TOUCH_BUTTON);
#endif
}

bool setCameraPower(bool enable)
{
    static bool started = false;
    if (!started) {
        Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL);
        Wire.beginTransmission(0x28);
        if (Wire.endTransmission() != 0) {
            log_e("Camera power chip not found!");
            return false;
        }
    }
    started = true;

    uint8_t vdd[] = {
        0x03,   /*reg address*/
        0x7C,   /*REG03 DVDD1 1.496V*/
        0x7C,   /*REG04 DVDD2 1.496v*/
        0xCA,   /*REG05 AVDD1 3.0V*/
        0xB1    /*REG06 AVDD2 2.8V*/
    };

    Wire.beginTransmission(0x28);
    Wire.write(vdd, sizeof(vdd) / sizeof(vdd[0]));
    Wire.endTransmission();

    uint8_t control[] = {0x0E, 0x0F};
    if (!enable) {
        control[1] = 0x00;
    }

    Wire.beginTransmission(0x28);
    Wire.write(control, sizeof(control) / sizeof(control[0]));
    Wire.endTransmission();

    if (enable) {
        /*
        * Maximize the use of GPIO. No GPIO is assigned to the camera reset pin, so the camera is reset by powering on again.
        */
        control[1] = 0x00;
        Wire.beginTransmission(0x28);
        Wire.write(control, sizeof(control) / sizeof(control[0]));
        Wire.endTransmission();
        delay(300);
        control[1] = 0x0F;
        Wire.beginTransmission(0x28);
        Wire.write(control, sizeof(control) / sizeof(control[0]));
        Wire.endTransmission();
        delay(100);
    }
    return true;
}

bool LilyGo_Glass::initCamera()
{
    if (!setCameraPower(true)) {
        log_e("Camera power on failed\n");
        return false;
    }

    esp_err_t err = esp_camera_init(&_camera_config);
    if (err != ESP_OK) {
        log_e("Camera init failed with error 0x%x", err);
        _camera_detected = false;
        return false;
    }
    _camera_detected = true;
    log_d("Camera init succeeded\n");
    return true;
}

void LilyGo_Glass::tone()
{
    if (!_es8311_detected) {
        log_e("es8311 not detected!");
        return;
    }

    rtttlFile->open(tone_rtttl, strlen(tone_rtttl));
    i2sRtttl->begin(rtttlFile, audioOut);
    while (1) {
        if (i2sRtttl->isRunning()) {
            if (!i2sRtttl->loop()) {
                i2sRtttl->stop();
                break;
            }
        } else {
            break;
        }
    }
}

void LilyGo_Glass::getAudioLevels(int *leftLevel, int *rightLevel)
{
    static int16_t buffer[256];
    size_t bytesRead = 0;
    if (leftLevel == nullptr || rightLevel == nullptr) {
        return;
    }
    if (!_es7210_detected) {
        return;
    }
    i2s_read(MIC_I2S_PORT, buffer, sizeof(buffer), &bytesRead, pdTICKS_TO_MS(100));
    if (bytesRead > 0) {
        int samples = bytesRead / sizeof(int16_t);
        if (samples >= 2) {
            long leftSum = 0, rightSum = 0;
            for (int i = 0; i < samples; i += 2) {
                leftSum += abs(buffer[i]);
                rightSum += abs(buffer[i + 1]);
            }
            int frameCount = samples / 2;
            *leftLevel = leftSum / frameCount;
            *rightLevel = rightSum / frameCount;
            return;
        }
    }
    *leftLevel = 0;
    *rightLevel = 0;
}

bool LilyGo_Glass::initI2S()
{
    i2s_driver_uninstall(MIC_I2S_PORT);

    i2s_mode_t mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX);
    i2s_config_t i2s_config_dac = {
        .mode = mode,
        .sample_rate = 44100,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 128,
        .use_apll = 0,
        .tx_desc_auto_clear = true,
        .fixed_mclk = true,
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 4, 0)
        .mclk_multiple = I2S_MCLK_MULTIPLE_256,
        .bits_per_chan = I2S_BITS_PER_CHAN_DEFAULT
#endif
    };
    if (i2s_driver_install(MIC_I2S_PORT, &i2s_config_dac, 0, NULL) != ESP_OK) {
        return false;
    }


    i2s_pin_config_t pins = {
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 4, 0)
        .mck_io_num = I2S_MCLK,
#endif
        .bck_io_num = I2S_SCK,
        .ws_io_num = I2S_WS,
        .data_out_num = I2S_SDOUT,
        .data_in_num = I2S_SDIN
    };
    return i2s_set_pin(MIC_I2S_PORT, &pins) == ESP_OK;
}


void LilyGo_Glass::deinitI2S()
{
    i2s_driver_uninstall(MIC_I2S_PORT);
}


bool LilyGo_Glass::begin()
{
    pinMode(LORA_RST, OUTPUT);
    digitalWrite(LORA_RST, HIGH);
    pinMode(LORA_CS, OUTPUT);
    digitalWrite(LORA_CS, HIGH);

    // Initialize display
    initBUS();

#ifdef USE_BUILTIN_TOUCH
    // Initialize touch button
    touchAttachInterrupt(BOARD_TOUCH_BUTTON, touchISR, threshold);

    LilyGo_Button::init(BOARD_TOUCH_BUTTON, 50, touchPadReadFunction);
#endif

    Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL);

    // 0x24: PCA9570
    // 0x6B: BQ25896
    // 0x40: ES7210
    // 0x18: ES8311
    // 0x55: BQ27220

    bool res = gauge.begin(Wire);
    _gauge_online = res;
    if (res) {
        log_d("Gauge initialized successfully\n");
    } else {
        log_e("Gauge initialization failed\n");
    }

    res  = expander.begin();
    if (res) {
        log_d("Expander initialized successfully\n");
    } else {
        log_e("Expander initialization failed\n");
    }

    expander.writePort(0xFF);

    _es8311_detected = false;
    _es7210_detected = false;
    Wire.beginTransmission(_ES8311_ADDRESS);
    if (Wire.endTransmission() == 0) {
        _es8311_detected = true;
    }

    Wire.beginTransmission(_ES7210_ADDRESS);
    if (Wire.endTransmission() == 0) {
        _es7210_detected = true;
    }

#ifndef USE_ESP_CODEC_LIB
    PinsAudioBoard.addI2C(PinFunction::CODEC, Wire);
#endif

    if (_es8311_detected) {

#ifdef USE_ESP_CODEC_LIB
        codec.setPins(I2S_MCLK, I2S_SCK, I2S_WS, I2S_SDOUT, I2S_SDIN);
        if (codec.begin(Wire, 0x18, CODEC_TYPE_ES8311)) {
            log_i("Codec init succeeded");
            codec.setGain(20);
            codec.setVolume(100);
        } else {
            log_e("Warning: Failed to find Codec");
        }
#else
        PinsAudioBoard.addI2S(PinFunction::CODEC, I2S_MCLK, I2S_SCK, I2S_WS, I2S_SDOUT, I2S_SDIN);
        CodecConfig audio_dac_cfg;
        audio_dac_cfg.input_device = ADC_INPUT_NONE;
        audio_dac_cfg.output_device = DAC_OUTPUT_ALL;
        audio_dac_cfg.i2s.bits = BIT_LENGTH_16BITS;
        audio_dac_cfg.i2s.rate = RATE_8K;
        if (audioOutputDev.begin(audio_dac_cfg)) {
            log_d("Audio DAC initialized successfully\n");
        } else {
            log_e("Audio DAC initialization failed\n");
        }
        audioOutputDev.setVolume(50);
        audioOutputDev.setMute(false);
#endif

        audioOut = new AudioOutputI2S(MIC_I2S_PORT, AudioOutputI2S::EXTERNAL_I2S);
        audioOut->SetPinout(I2S_SCK, I2S_WS, I2S_SDOUT, I2S_MCLK);
        audioOut->SetGain(1);


        rtttlFile = new AudioFileSourcePROGMEM(rudolph, strlen(rudolph));
        i2sRtttl = new AudioGeneratorRTTTL();
        i2sRtttl->begin(rtttlFile, audioOut);

        while (1) {
            if (i2sRtttl->isRunning()) {
                if (!i2sRtttl->loop()) {
                    i2sRtttl->stop();
                    break;
                }
            } else {
                break;
            }
        }
    }

    if (_es7210_detected) {

#ifndef USE_ESP_CODEC_LIB
        CodecConfig audio_adc_cfg;
        audio_adc_cfg.input_device = ADC_INPUT_LINE1;
        audio_adc_cfg.output_device = DAC_OUTPUT_NONE;
        audio_adc_cfg.i2s.bits = BIT_LENGTH_16BITS;
        audio_adc_cfg.i2s.rate = RATE_44K;
        if (audioInputDev.begin(audio_adc_cfg)) {
            log_d("Audio ADC initialized successfully\n");
        } else {
            log_e("Audio ADC initialization failed\n");
        }
#endif
    }


    res = ppm.init(Wire);
    if (res) {
        log_d("Power management initialized successfully\n");

        // Reset PPM
        ppm.resetDefault();

        // Set the charging target voltage full voltage to 4288mV
        ppm.setChargeTargetVoltage(4288);

        // The charging current should not be greater than half of the battery capacity.
        ppm.setChargerConstantCurr(256);

        // Enable measure
        ppm.enableMeasure();

    } else {
        log_e("Power management initialization failed\n");
    }

    res = initCamera();
    if (res) {
        log_d("Camera initialized successfully\n");
    } else {
        log_e("Camera initialization failed\n");
    }

    expander.digitalWrite(1, HIGH);

    return true;
}

void LilyGo_Glass::update()
{
#ifdef USE_BUILTIN_TOUCH
    LilyGo_Button::update();
#endif
}

void LilyGo_Glass::setBrightness(uint8_t level)
{
    lcd_cmd_t t = {0x51, {level}, 1};
    writeCommand(t.addr, t.param, t.len);
    _brightness = level;
}

uint8_t LilyGo_Glass::getBrightness()
{
    return _brightness;
}

void LilyGo_Glass::setRotation(uint8_t r)
{
    uint8_t write_data = 0x00;
    switch (r) {
    case 1: // jd9613 only has 1/2RAM and cannot be rotated
        // write_data = LCD_CMD_MX_BIT | LCD_CMD_MV_BIT | LCD_CMD_RGB;
        // _width = JD9613_HEIGHT;
        // _height = JD9613_WIDTH;

        write_data = LCD_CMD_RGB;
        _width = JD9613_WIDTH;
        _height = JD9613_HEIGHT;
        break;
    case 2:
        write_data = LCD_CMD_MY_BIT | LCD_CMD_MX_BIT | LCD_CMD_RGB;
        _width = JD9613_WIDTH;
        _height = JD9613_HEIGHT;
        break;
    case 3: // jd9613 only has 1/2RAM and cannot be rotated
        // write_data = LCD_CMD_MY_BIT | LCD_CMD_MV_BIT | LCD_CMD_RGB;
        // _width = JD9613_HEIGHT;
        // _height = JD9613_WIDTH;

        write_data = LCD_CMD_RGB;
        _width = JD9613_WIDTH;
        _height = JD9613_HEIGHT;
        break;
    default: // case 0:
        write_data = LCD_CMD_RGB;
        _width = JD9613_WIDTH;
        _height = JD9613_HEIGHT;
        break;
    }

    if (_flipHorizontal) {
        write_data |= (0x01 << 1); //Flip Horizontal
    }
    // write_data |= 0x01; //Flip Vertical
    _rotation = r;
    log_i("set_rotation:%d write reg :0x%X , data : 0x%X Width:%d Height:%d", r, LCD_CMD_MADCTL, write_data, _width, _height);
    writeCommand(LCD_CMD_MADCTL, &write_data, 1);
}

uint8_t LilyGo_Glass::getRotation()
{
    return (_rotation);
}

void LilyGo_Glass::setAddrWindow(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye)
{
    uint8_t data1[] = {lowByte(xs >> 8), lowByte(xs), lowByte((xs + xe - 1) >> 8), lowByte(xs + xe - 1)};
    writeCommand(LCD_CMD_CASET, data1, 4);
    uint8_t data2[] = {lowByte(ys >> 8), lowByte(ys), lowByte((ys + ye - 1) >> 8), lowByte(ys + ye - 1)};
    writeCommand( LCD_CMD_RASET, data2, 4);
    writeCommand( LCD_CMD_RAMWR, NULL, 0);
}

void LilyGo_Glass::flipHorizontal(bool enable)
{
    _flipHorizontal = enable;
}

void LilyGo_Glass::pushColors(uint16_t *pdat, uint32_t length)
{
    if (!pdat || length == 0)return ;
    digitalWrite(BOARD_DISP_CS, LOW);
    SPI.beginTransaction(SPISettings(_spi_freq, MSBFIRST, SPI_MODE0));
    digitalWrite(BOARD_DISP_DC, HIGH);
    SPI.writeBytes((uint8_t *)pdat, length * sizeof(uint16_t));
    SPI.endTransaction();
    digitalWrite(BOARD_DISP_CS, HIGH);
}

void LilyGo_Glass::pushColors(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end, uint16_t *color_data)
{

    uint32_t width = x_start + x_end;
    uint32_t height = y_start + y_end;
    uint32_t _x = x_start,
             _y = y_start,
             _xe = width,
             _ye = height;
    size_t write_colors_bytes = width * height * sizeof(uint16_t);
    uint16_t *data_ptr = (uint16_t *)color_data;


#ifdef SW_ROTATION
    bool sw_rotation = false;
    switch (_rotation) {
    case 1:
    case 3:
        sw_rotation = true;
        break;
    default:
        sw_rotation = false;
        break;
    }

    if (sw_rotation) {
        _x = JD9613_WIDTH - (y_start + height);
        _y = x_start;
        _xe = height;
        _ye = width;
    }
#endif

    // Direction 2 requires offset pixels
    if (_rotation == 2) {
        _x += 2;
        _xe += 2;
    }

    setAddrWindow(_x, _y, _xe, _ye);

#ifdef SW_ROTATION
    if (sw_rotation) {
        int index = 0;
        uint16_t *pdat = (uint16_t *)color_data;
        for (uint16_t j = 0; j < width; j++) {
            for (uint16_t i = 0; i < height; i++) {
                _frame_buffer[index++] = pdat[width * (height - i - 1) + j];
            }
        }
        data_ptr = _frame_buffer;
    }
#endif
    writeCommand(LCD_CMD_RAMWR, (uint8_t *)data_ptr, write_colors_bytes);
}

bool LilyGo_Glass::initBUS()
{
    _frame_buffer = (uint16_t *)ps_malloc(JD9613_WIDTH * JD9613_HEIGHT * sizeof(uint16_t));
    assert(_frame_buffer);

    SPI.begin(BOARD_DISP_SCK, BOARD_DISP_MISO, BOARD_DISP_MOSI);

    pinMode(BOARD_DISP_CS, OUTPUT);
    pinMode(BOARD_DISP_RST, OUTPUT);
    pinMode(BOARD_DISP_DC, OUTPUT);

    digitalWrite(BOARD_DISP_RST, LOW);
    delay(100);
    digitalWrite(BOARD_DISP_RST, HIGH);
    delay(100);

    const lcd_cmd_t *t = jd9613_cmd;
    for (uint32_t i = 0; i < (sizeof(jd9613_cmd) / sizeof(lcd_cmd_t)); i++) {
        writeCommand(jd9613_cmd[i].addr, (uint8_t*)jd9613_cmd[i].param, (jd9613_cmd[i].len - 1) & 0x7F);
        if (t[i].len & 0x80) {
            delay(120);
        }
    }

    setRotation(0);

    log_d("DISPLAY RESOLUTION: %dx%d\n", _width, _height);

    return true;
}


void LilyGo_Glass::writeCommand(uint32_t cmd, uint8_t *pdat, uint32_t length)
{
    digitalWrite(BOARD_DISP_CS, LOW);
    SPI.beginTransaction(SPISettings(_spi_freq, MSBFIRST, SPI_MODE0));
    digitalWrite(BOARD_DISP_DC, LOW);
    SPI.write(cmd);
    digitalWrite(BOARD_DISP_DC, HIGH);
    SPI.endTransaction();
    digitalWrite(BOARD_DISP_CS, HIGH);

    if (!pdat || length == 0)return ;
    digitalWrite(BOARD_DISP_CS, LOW);
    SPI.beginTransaction(SPISettings(_spi_freq, MSBFIRST, SPI_MODE0));
    digitalWrite(BOARD_DISP_DC, HIGH);
    SPI.writeBytes(pdat, length);
    SPI.endTransaction();
    digitalWrite(BOARD_DISP_CS, HIGH);
}

void LilyGo_Glass::enableTouchWakeup(uint32_t threshold)
{
    touchSleepWakeUpEnable(BOARD_TOUCH_BUTTON, threshold);

    esp_sleep_enable_touchpad_wakeup();
}


void writeToDevice(uint8_t address, uint8_t reg, uint8_t value)
{
    Wire.beginTransmission(address);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}

void LilyGo_Glass::sleep()
{
    lcd_cmd_t t = {0x10, {0x00}, 1}; //Sleep in
    writeCommand(t.addr, t.param, t.len);

    esp_camera_deinit();

    if (!setCameraPower(false)) {
        log_e("Failed to turn off camera power");
    }

    ppm.disableMeasure();


    i2s_driver_uninstall(MIC_I2S_PORT);

    if (_es8311_detected) {
        // audioOutputDev.end();

#ifdef USE_ESP_CODEC_LIB
        codec.end();
#else
        writeToDevice(_ES8311_ADDRESS, 0x32, 0x00);
        writeToDevice(_ES8311_ADDRESS, 0x17, 0x00);
        writeToDevice(_ES8311_ADDRESS, 0x0E, 0xFF);
        writeToDevice(_ES8311_ADDRESS, 0x12, 0x02);
        writeToDevice(_ES8311_ADDRESS, 0x14, 0x00);
        writeToDevice(_ES8311_ADDRESS, 0x0D, 0xFA);
        writeToDevice(_ES8311_ADDRESS, 0x15, 0x00);
        writeToDevice(_ES8311_ADDRESS, 0x02, 0x10);
        writeToDevice(_ES8311_ADDRESS, 0x00, 0x00);
        writeToDevice(_ES8311_ADDRESS, 0x00, 0x1F);
        writeToDevice(_ES8311_ADDRESS, 0x01, 0x30);
        writeToDevice(_ES8311_ADDRESS, 0x01, 0x00);
        writeToDevice(_ES8311_ADDRESS, 0x45, 0x00);
        writeToDevice(_ES8311_ADDRESS, 0x0D, 0xFC);
        writeToDevice(_ES8311_ADDRESS, 0x02, 0x00);
#endif
    }

    if (_es7210_detected) {
        // avg:600uA max 880~900uA
        writeToDevice(_ES7210_ADDRESS, 0x4B, 0xFF);
        writeToDevice(_ES7210_ADDRESS, 0x4C, 0xFF);
        writeToDevice(_ES7210_ADDRESS, 0x0B, 0xD0);
        writeToDevice(_ES7210_ADDRESS, 0x40, 0x80);
        writeToDevice(_ES7210_ADDRESS, 0x01, 0x7F);
        writeToDevice(_ES7210_ADDRESS, 0x06, 0x07);
    }

    // https://e2e.ti.com/support/power-management-group/power-management/f/power-management-forum/741260/linux-linux-bq27220---how-to-enter-shutdown-mode
    // BQ27220 gauge can't shutdown

    expander.digitalWrite(0, LOW);
    expander.digitalWrite(1, LOW);
    expander.digitalWrite(2, LOW);
    expander.digitalWrite(3, LOW);

    Wire.end();

    SPI.end();

    const uint8_t pins[] = {
        BOARD_DISP_CS,
        BOARD_DISP_SCK,
        BOARD_DISP_MISO,
        BOARD_DISP_MOSI,
        BOARD_DISP_DC,
        BOARD_DISP_RST,

        BOARD_I2C_SDA,
        BOARD_I2C_SCL,

        RESET_GPIO_NUM,
        XCLK_GPIO_NUM,
        SIOD_GPIO_NUM,
        SIOC_GPIO_NUM,
        VSYNC_GPIO_NUM,
        HREF_GPIO_NUM,
        PCLK_GPIO_NUM,
        Y9_GPIO_NUM,
        Y8_GPIO_NUM,
        Y7_GPIO_NUM,
        Y6_GPIO_NUM,
        Y5_GPIO_NUM,
        Y4_GPIO_NUM,
        Y3_GPIO_NUM,
        Y2_GPIO_NUM,

        I2S_WS,
        I2S_SCK,
        I2S_MCLK,
        I2S_SDOUT,
        I2S_SDIN,

        LORA_CS,
        LORA_RST,
        LORA_BUSY,
        LORA_IRQ,
    };

    for (auto pin : pins) {
        gpio_reset_pin((gpio_num_t )pin);
        pinMode(pin, INPUT);
    }

    Serial.end();

    delay(500);

    esp_deep_sleep_start();
}

void LilyGo_Glass::wakeup()
{
    lcd_cmd_t t = {0x11, {0x00}, 1};// Sleep Out
    writeCommand(t.addr, t.param, t.len);
}

uint16_t  LilyGo_Glass::width()
{
#ifdef SW_ROTATION
    switch (_rotation) {
    case 1:
    case 3:
        return _height;
        break;
    default:
        break;
    }
#endif
    return _width;
}

uint16_t  LilyGo_Glass::height()
{
#ifdef SW_ROTATION
    switch (_rotation) {
    case 1:
    case 3:
        return _width;
        break;
    default:
        break;
    }
#endif
    return _height;
}

bool LilyGo_Glass::hasTouch()
{
    return false;
}

uint8_t LilyGo_Glass::getPoint(int16_t *x, int16_t *y, uint8_t get_point )
{
    return 0;
}

uint16_t LilyGo_Glass::getBattVoltage(void)
{
    uint16_t mv = 0;
    if (_gauge_online) {
        if (millis() - _last_ref_data > _ref_data_interval) {
            _last_ref_data = millis();
            gauge.refresh();

        }
        mv = gauge.getVoltage();
    }
    return mv;
}

int LilyGo_Glass::getBatteryPercent()
{
    int percent = 0;
    if (_gauge_online) {
        if (millis() - _last_ref_data > _ref_data_interval) {
            _last_ref_data = millis();
            gauge.refresh();

        }
        percent = gauge.getStateOfCharge();
    }
    return percent;
}

bool LilyGo_Glass::needFullRefresh()
{
    return true;
}

bool LilyGo_Glass::initMicrophone()
{
    initI2S();
    return true;
}

bool LilyGo_Glass::readStream(void *dest, size_t size, size_t *bytes_read, TickType_t ticks_to_wait)
{
    return i2s_read(MIC_I2S_PORT, dest, size, bytes_read, ticks_to_wait) == ESP_OK;
}

bool LilyGo_Glass::writeStream(const void *src, size_t size, size_t *bytes_written, TickType_t ticks_to_wait)
{
    return i2s_write(MIC_I2S_PORT, src, size, bytes_written, ticks_to_wait) == ESP_OK;
}

LilyGo_Glass glass;
