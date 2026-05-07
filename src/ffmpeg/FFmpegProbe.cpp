#include "ffmpeg/FFmpegProbe.h"

extern "C" {
#include <libavcodec/avcodec.h>
}

#include <string>

namespace playerlab::ffmpeg {

std::string FFmpegProbe::versionString() {
    return "libavcodec=" + std::to_string(avcodec_version());
}

}  // namespace playerlab::ffmpeg
