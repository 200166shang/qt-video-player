#pragma once

#include <string>

namespace playerlab::ffmpeg {

class FFmpegProbe {
public:
    static std::string versionString();
};

}  // namespace playerlab::ffmpeg
