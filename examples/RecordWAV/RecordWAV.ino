/**
 * @file      GlassWindown.ino
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2024  ShenZhen XinYuan Electronic Technology Co., Ltd
 * @date      2024-02-22
 *
 */
#include <LilyGo_GlassV3.h>
#include <LV_Helper.h>
#include "_wav_header.h"

// The resolution of the non-magnified side of the glasses reflection area is about 126x126,
// and the magnified area is smaller than 126x126
#define GlassViewableWidth              126
#define GlassViewableHeight             126


const int WAVE_HEADER_SIZE = PCM_WAV_HEADER_SIZE;

bool playWAV(uint8_t *data, size_t len)
{
    pcm_wav_header_t *header = (pcm_wav_header_t *)data;
    if (header->fmt_chunk.audio_format != 1) {
        log_e("Audio format is not PCM!");
        return false;
    }
    wav_data_chunk_t *data_chunk = &header->data_chunk;
    size_t data_offset = 0;
    while (memcmp(data_chunk->subchunk_id, "data", 4) != 0) {
        log_d(
            "Skip chunk: %c%c%c%c, len: %lu", data_chunk->subchunk_id[0], data_chunk->subchunk_id[1], data_chunk->subchunk_id[2], data_chunk->subchunk_id[3],
            data_chunk->subchunk_size + 8
        );
        data_offset += data_chunk->subchunk_size + 8;
        data_chunk = (wav_data_chunk_t *)(data + WAVE_HEADER_SIZE + data_offset - 8);
    }
    log_d(
        "Play WAV: rate:%lu, bits:%d, channels:%d, size:%lu", header->fmt_chunk.sample_rate, header->fmt_chunk.bits_per_sample, header->fmt_chunk.num_of_channels,
        data_chunk->subchunk_size
    );

    CodecConfig cfg;
    cfg.i2s.rate = RATE_16K;
    cfg.i2s.bits = BIT_LENGTH_16BITS;
    audioOutputDev.setConfig(cfg);
    audioOutputDev.setVolume(100);
    size_t bytes_written = 0;
    glass.writeStream(data + WAVE_HEADER_SIZE + data_offset, data_chunk->subchunk_size, &bytes_written);
    return true;
}


bool recordWAV(size_t rec_seconds, uint8_t**output, size_t *out_size)
{
    uint16_t sample_rate = 16000;
    uint8_t num_channels = 1;
    uint16_t sample_width = 16;
    size_t rec_size = rec_seconds * ((sample_rate * (sample_width / 8)) * num_channels);
    const pcm_wav_header_t wav_header = PCM_WAV_HEADER_DEFAULT(rec_size, sample_width, sample_rate, num_channels);
    *out_size = 0;
    Serial.printf("Record WAV: rate:%lu, bits:%u, channels:%u, size:%lu", sample_rate, sample_width, num_channels, rec_size);
    uint8_t *wav_buf = (uint8_t *)ps_malloc(rec_size + PCM_WAV_HEADER_SIZE);
    if (wav_buf == NULL) {
        log_e("Failed to allocate WAV buffer with size %u", rec_size + PCM_WAV_HEADER_SIZE);
        return false;
    }
    memcpy(wav_buf, &wav_header, PCM_WAV_HEADER_SIZE);

    CodecConfig cfg;
    cfg.i2s.rate = RATE_16K;
    cfg.i2s.bits = BIT_LENGTH_16BITS;
    audioInputDev.setConfig(cfg);
    size_t bytes_read = 0;
    glass.readStream(wav_buf + PCM_WAV_HEADER_SIZE, rec_size, &bytes_read);
    *out_size = rec_size + PCM_WAV_HEADER_SIZE;
    *output = wav_buf;
    return true;
}

void setup()
{
    bool rslt = false;

    // Turn on debugging message output, Arduino IDE users please put
    // Tools -> USB CDC On Boot -> Enable, otherwise there will be no output
    Serial.begin(115200);

    // Initialization screen and peripherals
    rslt = glass.begin();
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
    // Set window border width zero
    lv_obj_set_style_border_width(window, 0, 0);
    // Set display window size
    lv_obj_set_size(window, GlassViewableWidth, GlassViewableHeight);
    // Set window position
    lv_obj_align(window, LV_ALIGN_BOTTOM_MID, 0, 0);


    lv_obj_t *label = lv_label_create(window);        /*Add a label the current screen*/
    lv_label_set_text(label, "recordWAV");          /*Set label text*/
    lv_obj_center(label);                             /*Set center alignment*/
    lv_timer_handler(); 

    uint8_t *wav_buf = NULL;
    size_t out_size = 0;

    glass.initMicrophone();
    
    recordWAV(5, &wav_buf, &out_size);

    if (wav_buf == NULL) {
        lv_label_set_text(label, "Failed to record WAV");
        Serial.println("Failed to record WAV");
        return;
    }

    playWAV(wav_buf, out_size);

}



void loop()
{
    // lvgl task processing should be placed in the loop function
    lv_timer_handler();
    delay(2);
}






