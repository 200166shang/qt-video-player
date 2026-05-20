#include "ffmpeg/FFmpegReadWorker.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

namespace {

std::string ffmpegErrorToString(const int errNum) {
    char errBuf[AV_ERROR_MAX_STRING_SIZE] = {0};
    av_strerror(errNum, errBuf, sizeof(errBuf));
    return std::string(errBuf);
}

}  // namespace

namespace playerlab::ffmpeg {

FFmpegReadWorker::~FFmpegReadWorker() {
    stop();
}

bool FFmpegReadWorker::open(const playerlab::core::MediaSource& source, playerlab::core::MediaInfo& outInfo,
                            std::string& outError, const double startPositionSec) {
    stop();

    const int openRet = avformat_open_input(&formatContext_, source.uri.c_str(), nullptr, nullptr);
    if (openRet < 0) {
        outError = "open input failed: " + ffmpegErrorToString(openRet);
        return false;
    }

    const int streamRet = avformat_find_stream_info(formatContext_, nullptr);
    if (streamRet < 0) {
        outError = "find stream info failed: " + ffmpegErrorToString(streamRet);
        stop();
        return false;
    }

    videoStreamIndex_ = av_find_best_stream(formatContext_, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    audioStreamIndex_ = av_find_best_stream(formatContext_, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (videoStreamIndex_ < 0 && audioStreamIndex_ < 0) {
        outError = "no playable audio/video stream found";
        stop();
        return false;
    }

    if (startPositionSec > 0.0 && !seekTo(startPositionSec, outError)) {
        stop();
        return false;
    }

    outInfo = buildMediaInfo(source, formatContext_);
    readFinished_.store(false);
    return true;
}

void FFmpegReadWorker::start(PacketQueue<AVPacket*>* videoPacketQueue, PacketQueue<AVPacket*>* audioPacketQueue) {
    videoPacketQueue_ = videoPacketQueue;
    audioPacketQueue_ = audioPacketQueue;
    readFinished_.store(false);
    running_.store(true);
    readThread_ = std::thread([this]() { readLoop(); });
}

void FFmpegReadWorker::stop() {
    running_.store(false);

    if (readThread_.joinable()) {
        readThread_.join();
    }

    videoPacketQueue_ = nullptr;
    audioPacketQueue_ = nullptr;
    if (formatContext_ != nullptr) {
        avformat_close_input(&formatContext_);
    }
    videoStreamIndex_ = -1;
    audioStreamIndex_ = -1;
    readFinished_.store(false);
}

const AVCodecParameters* FFmpegReadWorker::videoCodecParameters() const {
    if (formatContext_ == nullptr || videoStreamIndex_ < 0) {
        return nullptr;
    }
    return formatContext_->streams[videoStreamIndex_]->codecpar;
}

const AVCodecParameters* FFmpegReadWorker::audioCodecParameters() const {
    if (formatContext_ == nullptr || audioStreamIndex_ < 0) {
        return nullptr;
    }
    return formatContext_->streams[audioStreamIndex_]->codecpar;
}

AVRational FFmpegReadWorker::videoTimeBase() const {
    if (formatContext_ == nullptr || videoStreamIndex_ < 0) {
        return AVRational{0, 1};
    }
    return formatContext_->streams[videoStreamIndex_]->time_base;
}

AVRational FFmpegReadWorker::audioTimeBase() const {
    if (formatContext_ == nullptr || audioStreamIndex_ < 0) {
        return AVRational{0, 1};
    }
    return formatContext_->streams[audioStreamIndex_]->time_base;
}

bool FFmpegReadWorker::seekTo(const double startPositionSec, std::string& outError) {
    const int64_t targetPts = static_cast<int64_t>(startPositionSec * AV_TIME_BASE);
    const int seekRet = av_seek_frame(formatContext_, -1, targetPts, AVSEEK_FLAG_BACKWARD);
    if (seekRet < 0) {
        outError = "seek input failed: " + ffmpegErrorToString(seekRet);
        return false;
    }
    return true;
}

void FFmpegReadWorker::readLoop() {
    while (running_.load()) {
        AVPacket* packet = av_packet_alloc();
        if (packet == nullptr) {
            break;
        }

        const int readRet = av_read_frame(formatContext_, packet);
        if (readRet < 0) {
            av_packet_free(&packet);
            break;
        }

        PacketQueue<AVPacket*>* targetQueue = nullptr;
        if (packet->stream_index == videoStreamIndex_) {
            targetQueue = videoPacketQueue_;
        } else if (packet->stream_index == audioStreamIndex_) {
            targetQueue = audioPacketQueue_;
        }

        if (targetQueue == nullptr) {
            av_packet_free(&packet);
            continue;
        }

        if (!targetQueue->waitPush(packet)) {
            av_packet_free(&packet);
            break;
        }
    }

    if (videoPacketQueue_ != nullptr && videoStreamIndex_ >= 0) {
        if (AVPacket* flushPacket = makeFlushPacket(videoStreamIndex_); flushPacket != nullptr) {
            videoPacketQueue_->waitPush(flushPacket);
        }
    }
    if (audioPacketQueue_ != nullptr && audioStreamIndex_ >= 0) {
        if (AVPacket* flushPacket = makeFlushPacket(audioStreamIndex_); flushPacket != nullptr) {
            audioPacketQueue_->waitPush(flushPacket);
        }
    }

    readFinished_.store(true);
}

playerlab::core::MediaInfo FFmpegReadWorker::buildMediaInfo(const playerlab::core::MediaSource& source,
                                                            const AVFormatContext* formatContext) {
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
            const AVRational fpsR = av_guess_frame_rate(const_cast<AVFormatContext*>(formatContext), stream, nullptr);
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

    return info;
}

AVPacket* FFmpegReadWorker::makeFlushPacket(const int streamIndex) {
    AVPacket* flushPacket = av_packet_alloc();
    if (flushPacket == nullptr) {
        return nullptr;
    }
    flushPacket->data = nullptr;
    flushPacket->size = 0;
    flushPacket->stream_index = streamIndex;
    return flushPacket;
}

}  // namespace playerlab::ffmpeg
