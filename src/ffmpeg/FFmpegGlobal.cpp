#include "ffmpeg/FFmpegGlobal.h"

extern "C" {
#include <libavformat/avformat.h>
}

#include <mutex>

namespace playerlab::ffmpeg {

void FFmpegGlobal::initialize() {
    static std::once_flag once;
    std::call_once(once, []() {
        avformat_network_init();
    });
}

}  // namespace playerlab::ffmpeg
