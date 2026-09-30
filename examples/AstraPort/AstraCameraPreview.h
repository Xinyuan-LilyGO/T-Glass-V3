#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace astra_camera {

inline bool copyCenteredRgb565Frame(std::uint16_t *output,
                                    std::uint16_t outputWidth,
                                    std::uint16_t outputHeight,
                                    const std::uint8_t *frame,
                                    std::size_t frameLength,
                                    std::uint16_t frameWidth,
                                    std::uint16_t frameHeight) {
    if (output == nullptr || frame == nullptr || outputWidth == 0 || outputHeight == 0 ||
        frameWidth == 0 || frameHeight == 0) {
        return false;
    }

    const std::size_t sourcePixelCount = static_cast<std::size_t>(frameWidth) * frameHeight;
    if (sourcePixelCount > static_cast<std::size_t>(-1) / sizeof(std::uint16_t)) return false;
    const std::size_t sourceBytes = sourcePixelCount * sizeof(std::uint16_t);
    if (frameLength < sourceBytes) return false;

    std::fill(output,
              output + static_cast<std::size_t>(outputWidth) * outputHeight,
              static_cast<std::uint16_t>(0));

    const std::uint16_t copyWidth = std::min(outputWidth, frameWidth);
    const std::uint16_t copyHeight = std::min(outputHeight, frameHeight);
    const std::uint16_t sourceX = static_cast<std::uint16_t>((frameWidth - copyWidth) / 2);
    const std::uint16_t sourceY = static_cast<std::uint16_t>((frameHeight - copyHeight) / 2);
    const std::uint16_t destinationX =
        static_cast<std::uint16_t>((outputWidth - copyWidth) / 2);
    const std::uint16_t destinationY =
        static_cast<std::uint16_t>((outputHeight - copyHeight) / 2);

    for (std::uint16_t row = 0; row < copyHeight; ++row) {
        const std::size_t sourceOffset =
            (static_cast<std::size_t>(sourceY + row) * frameWidth + sourceX) *
            sizeof(std::uint16_t);
        const std::size_t destinationOffset =
            static_cast<std::size_t>(destinationY + row) * outputWidth + destinationX;
        std::memcpy(output + destinationOffset,
                    frame + sourceOffset,
                    static_cast<std::size_t>(copyWidth) * sizeof(std::uint16_t));
    }

    return true;
}

}  // namespace astra_camera
