#include "ffmpeg/FFmpegDemuxer.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

#include <iomanip>
#include <sstream>

namespace {

std::string ffmpegErrorToString(const int errNum) {
    char errBuf[AV_ERROR_MAX_STRING_SIZE] = {0};
    av_strerror(errNum, errBuf, sizeof(errBuf));
    return std::string(errBuf);
}

}  // namespace

namespace playerlab::ffmpeg {

bool FFmpegDemuxer::open(const playerlab::core::MediaSource& source, playerlab::core::MediaInfo& outInfo,
                         std::string& outError) const {
    AVFormatContext* formatContext = nullptr;
    const int openRet = avformat_open_input(&formatContext, source.uri.c_str(), nullptr, nullptr);
    if (openRet < 0) {
        outError = "open input failed: " + ffmpegErrorToString(openRet);
        return false;
    }

    const int streamRet = avformat_find_stream_info(formatContext, nullptr);
    if (streamRet < 0) {
        outError = "find stream info failed: " + ffmpegErrorToString(streamRet);
        avformat_close_input(&formatContext);
        return false;
    }

    playerlab::core::MediaInfo info;
    info.filePath = source.uri;
    info.containerFormat = formatContext->iformat != nullptr ? formatContext->iformat->long_name : "unknown";
    if (formatContext->duration > 0) {
        info.durationMs = formatContext->duration / (AV_TIME_BASE / 1000);
    }

    for (unsigned int i = 0; i < formatContext->nb_streams; ++i) {
        AVStream* stream = formatContext->streams[i];
        const AVCodecParameters* codecPar = stream->codecpar;
        const AVCodecDescriptor* codecDesc = avcodec_descriptor_get(codecPar->codec_id);
        const std::string codecName = codecDesc != nullptr ? codecDesc->name : "unknown";

        if (codecPar->codec_type == AVMEDIA_TYPE_VIDEO && !info.hasVideo) {
            info.hasVideo = true;
            info.videoWidth = codecPar->width;
            info.videoHeight = codecPar->height;
            info.videoCodec = codecName;
            const AVRational fpsR = av_guess_frame_rate(formatContext, stream, nullptr);
            if (fpsR.den != 0 && fpsR.num != 0) {
                info.frameRate = static_cast<double>(fpsR.num) / static_cast<double>(fpsR.den);
            }
            continue;
        }

        if (codecPar->codec_type == AVMEDIA_TYPE_AUDIO && !info.hasAudio) {
            info.hasAudio = true;
            info.audioCodec = codecName;
            info.audioSampleRate = codecPar->sample_rate;
            info.audioChannels = codecPar->ch_layout.nb_channels;
        }
    }

    avformat_close_input(&formatContext);
    outInfo = info;
    return true;
}

}  // namespace playerlab::ffmpeg
