#pragma once
#include "dl_detect_base.hpp"
#include "dl_detect_espdet_postprocessor.hpp"

namespace hand_detect {
class ESPDet : public dl::detect::DetectImpl {
public:
    static constexpr float default_score_thr = 0.25;
    static constexpr float default_nms_thr = 0.5;
    ESPDet(const char *model_name, float score_thr, float nms_thr);
    bool isReady() const {
        return m_model != nullptr && m_image_preprocessor != nullptr &&
               m_postprocessor != nullptr && m_model->get_input() != nullptr;
    }
};
} // namespace hand_detect

class HandDetect : public dl::detect::DetectWrapper {
public:
    typedef enum {
        ESPDET_PICO_224_224_HAND,
    } model_type_t;
    HandDetect(model_type_t model_type = static_cast<model_type_t>(CONFIG_DEFAULT_HAND_DETECT_MODEL),
               bool lazy_load = true);
    bool isReady() const { return m_model != nullptr; }

private:
    void load_model() override;
    model_type_t m_model_type;
};
