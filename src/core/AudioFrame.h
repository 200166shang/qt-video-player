#pragma once

#include <cstdint>
#include <vector>

namespace playerlab::core {

enum class AudioSampleFormat {
    S16,
    Float32,
};

struct AudioFrame {
    std::vector<std::uint8_t> data;
    int sampleRate = 0;
    int channels = 0;
    AudioSampleFormat sampleFormat = AudioSampleFormat::S16;
    int sampleCount = 0;
    double ptsSec = 0.0;

    [[nodiscard]] bool isValid() const {
        return sampleRate > 0 && channels > 0 && sampleCount > 0 && !data.empty();
    }
};

}  // namespace playerlab::core
