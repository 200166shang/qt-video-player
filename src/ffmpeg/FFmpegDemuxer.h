#pragma once

#include <string>

#include "core/MediaInfo.h"
#include "core/MediaSource.h"

namespace playerlab::ffmpeg {

class FFmpegDemuxer {
public:
    bool open(const playerlab::core::MediaSource& source, playerlab::core::MediaInfo& outInfo,
              std::string& outError) const;
};

}  // namespace playerlab::ffmpeg
