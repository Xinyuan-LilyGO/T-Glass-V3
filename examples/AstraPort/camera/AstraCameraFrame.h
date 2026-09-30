#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace astra_camera {

struct FrameInfo {
    std::uint16_t width = 0;
    std::uint16_t height = 0;
    std::size_t length = 0;
};

inline bool copyJpegBytes(std::uint8_t *destination,
                          std::size_t capacity,
                          const std::uint8_t *source,
                          std::size_t length,
                          std::uint16_t width,
                          std::uint16_t height,
                          FrameInfo &info,
                          bool isJpeg = true) {
    if (!isJpeg || destination == nullptr || source == nullptr || length == 0 ||
        length > capacity || width == 0 || height == 0) {
        return false;
    }

    std::memcpy(destination, source, length);
    info.width = width;
    info.height = height;
    info.length = length;
    return true;
}

} // namespace astra_camera
