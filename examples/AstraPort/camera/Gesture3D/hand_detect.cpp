#include "hand_detect.hpp"
#include "esp_log.h"
#include <filesystem>
#include <new>

#if CONFIG_HAND_DETECT_MODEL_IN_FLASH_RODATA
extern const uint8_t hand_detect_espdl[]
    asm("_binary_examples_AstraPort_models_hand_detect_espdl_start");
static const char *path = (const char *)hand_detect_espdl;
#elif CONFIG_HAND_DETECT_MODEL_IN_FLASH_PARTITION
static const char *path = "hand_det";
#else
#if !defined(CONFIG_BSP_SD_MOUNT_POINT)
#define CONFIG_BSP_SD_MOUNT_POINT "/sdcard"
#endif
#endif
namespace hand_detect {
ESPDet::ESPDet(const char *model_name, float score_thr, float nms_thr)
{
    m_model = nullptr;
    m_image_preprocessor = nullptr;
    m_postprocessor = nullptr;
#if !CONFIG_HAND_DETECT_MODEL_IN_SDCARD
    m_model = new (std::nothrow)
        dl::Model(path, model_name, static_cast<fbs::model_location_type_t>(CONFIG_HAND_DETECT_MODEL_LOCATION));
#else
    auto sd_path = std::filesystem::path(CONFIG_BSP_SD_MOUNT_POINT) / CONFIG_HAND_DETECT_MODEL_SDCARD_DIR / model_name;
    m_model = new (std::nothrow) dl::Model(sd_path.c_str(), fbs::MODEL_LOCATION_IN_SDCARD);
#endif
    if (m_model == nullptr || m_model->get_input() == nullptr) {
        return;
    }
    m_model->minimize();
    m_image_preprocessor = new (std::nothrow)
        dl::image::ImagePreprocessor(m_model, {0, 0, 0}, {255, 255, 255});
    if (m_image_preprocessor == nullptr) {
        return;
    }
    m_image_preprocessor->enable_letterbox({114, 114, 114});
    m_postprocessor = new (std::nothrow) dl::detect::ESPDetPostProcessor(
        m_model, m_image_preprocessor, score_thr, nms_thr, 10, {{8, 8, 4, 4}, {16, 16, 8, 8}, {32, 32, 16, 16}});
}

} // namespace hand_detect

HandDetect::HandDetect(model_type_t model_type, bool lazy_load) : m_model_type(model_type)
{
    m_model = nullptr;
    switch (model_type) {
    case model_type_t::ESPDET_PICO_224_224_HAND:
        m_score_thr[0] = hand_detect::ESPDet::default_score_thr;
        m_nms_thr[0] = hand_detect::ESPDet::default_nms_thr;
        break;
    }
    if (lazy_load) {
        m_model = nullptr;
    } else {
        load_model();
    }
}

void HandDetect::load_model()
{
    switch (m_model_type) {
    case model_type_t::ESPDET_PICO_224_224_HAND:
#if CONFIG_FLASH_ESPDET_PICO_224_224_HAND || CONFIG_HAND_DETECT_MODEL_IN_SDCARD
    {
        auto *model = new (std::nothrow) hand_detect::ESPDet(
            "espdet_pico_224_224_hand.espdl", m_score_thr[0], m_nms_thr[0]);
        if (model != nullptr && model->isReady()) {
            m_model = model;
        } else {
            delete model;
            ESP_LOGE("hand_detect", "hand detector model is not ready");
        }
    }
#else
        ESP_LOGE(
            "hand_detect", "espdet_pico_224_224_hand is not selected in menuconfig.", m_score_thr[0], m_nms_thr[0]);
#endif
        break;
    }
}
