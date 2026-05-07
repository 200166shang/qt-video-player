#pragma once

#include <string>

namespace playerlab::core {

struct MediaInfo {
    std::string filePath;
    std::string containerFormat;
    int64_t durationMs = 0;

    bool hasVideo = false;
    int videoWidth = 0;
    int videoHeight = 0;
    std::string videoCodec;
    double frameRate = 0.0;

    bool hasAudio = false;
    std::string audioCodec;
    int audioSampleRate = 0;
    int audioChannels = 0;
};

}  // namespace playerlab::core
