#include <cstdint>
#include <cstring>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace {

constexpr std::uint32_t kCameraTaskStackDepth = 4096;

bool isCameraTask(const char *name) {
    return name != nullptr && std::strcmp(name, "cam_task") == 0;
}

} // namespace

extern "C" BaseType_t __real_xTaskCreatePinnedToCore(
    TaskFunction_t taskCode,
    const char *const taskName,
    const std::uint32_t stackDepth,
    void *const parameters,
    UBaseType_t priority,
    TaskHandle_t *const taskHandle,
    const BaseType_t coreId);

extern "C" BaseType_t __wrap_xTaskCreatePinnedToCore(
    TaskFunction_t taskCode,
    const char *const taskName,
    const std::uint32_t stackDepth,
    void *const parameters,
    UBaseType_t priority,
    TaskHandle_t *const taskHandle,
    const BaseType_t coreId) {
    const std::uint32_t adjustedDepth =
        isCameraTask(taskName) && stackDepth < kCameraTaskStackDepth
            ? kCameraTaskStackDepth
            : stackDepth;

    return __real_xTaskCreatePinnedToCore(taskCode,
                                          taskName,
                                          adjustedDepth,
                                          parameters,
                                          priority,
                                          taskHandle,
                                          coreId);
}
