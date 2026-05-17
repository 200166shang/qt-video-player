#include "ffmpeg/FFmpegAudioDecoder.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

#include <cmath>

namespace {

std::string ffmpegErrorToString(const int errNum) {
    char errBuf[AV_ERROR_MAX_STRING_SIZE] = {0};
    av_strerror(errNum, errBuf, sizeof(errBuf));
    return std::string(errBuf);
}

}  // namespace

namespace playerlab::ffmpeg {

FFmpegAudioDecoder::~FFmpegAudioDecoder() {
    stop();
}

bool FFmpegAudioDecoder::open(const playerlab::core::MediaSource& source, std::string& outError,
                              const double startPositionSec, const double playbackRate) {
    stop();
    packetQueue_.reset();
    frameQueue_.reset();
    demuxFinished_.store(false);
    decodeFinished_.store(false);

    if (!openInput(source.uri, outError, startPositionSec)) {
        return false;
    }
    if (!openAudioDecoder(outError, playbackRate)) {
        stop();
        return false;
    }

    running_.store(true);
    demuxThread_ = std::thread([this]() { demuxLoop(); });
    decodeThread_ = std::thread([this]() { decodeLoop(); });
    return true;
}

void FFmpegAudioDecoder::stop() {
    running_.store(false);
    packetQueue_.abort();

    if (demuxThread_.joinable()) {
        demuxThread_.join();
    }
    if (decodeThread_.joinable()) {
        decodeThread_.join();
    }

    AVPacket* packet = nullptr;
    while (packetQueue_.tryPop(packet)) {
        if (packet != nullptr) {
            av_packet_free(&packet);
        }
    }

    packetQueue_.clear();
    frameQueue_.clear();
    resampler_.close();

    if (codecContext_ != nullptr) {
        avcodec_free_context(&codecContext_);
    }
    if (formatContext_ != nullptr) {
        avformat_close_input(&formatContext_);
    }
    audioStreamIndex_ = -1;
    demuxFinished_.store(false);
    decodeFinished_.store(false);
}

bool FFmpegAudioDecoder::tryPopFrame(playerlab::core::AudioFrame& outFrame) {
    return frameQueue_.tryPop(outFrame);
}

bool FFmpegAudioDecoder::isDrained() const {
    return demuxFinished_.load() && decodeFinished_.load() && packetQueue_.empty() && frameQueue_.empty();
}

bool FFmpegAudioDecoder::openInput(const std::string& uri, std::string& outError, const double startPositionSec) {
    const int openRet = avformat_open_input(&formatContext_, uri.c_str(), nullptr, nullptr);
    if (openRet < 0) {
        outError = "open input failed: " + ffmpegErrorToString(openRet);
        return false;
    }

    const int streamRet = avformat_find_stream_info(formatContext_, nullptr);
    if (streamRet < 0) {
        outError = "find stream info failed: " + ffmpegErrorToString(streamRet);
        return false;
    }

    audioStreamIndex_ = av_find_best_stream(formatContext_, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (audioStreamIndex_ < 0) {
        outError = "audio stream not found";
        return false;
    }

    if (startPositionSec > 0.0) {
        AVStream* stream = formatContext_->streams[audioStreamIndex_];
        const int64_t targetPts = av_rescale_q(static_cast<int64_t>(startPositionSec * AV_TIME_BASE), AV_TIME_BASE_Q,
                                               stream->time_base);
        const int seekRet = av_seek_frame(formatContext_, audioStreamIndex_, targetPts, AVSEEK_FLAG_BACKWARD);
        if (seekRet < 0) {
            outError = "seek audio failed: " + ffmpegErrorToString(seekRet);
            return false;
        }
    }

    return true;
}

bool FFmpegAudioDecoder::openAudioDecoder(std::string& outError, const double playbackRate) {
    AVStream* stream = formatContext_->streams[audioStreamIndex_];
    const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (codec == nullptr) {
        outError = "audio decoder not found";
        return false;
    }

    codecContext_ = avcodec_alloc_context3(codec);
    if (codecContext_ == nullptr) {
        outError = "alloc codec context failed";
        return false;
    }

    const int parRet = avcodec_parameters_to_context(codecContext_, stream->codecpar);
    if (parRet < 0) {
        outError = "copy codec parameters failed: " + ffmpegErrorToString(parRet);
        return false;
    }

    const int openRet = avcodec_open2(codecContext_, codec, nullptr);
    if (openRet < 0) {
        outError = "open audio decoder failed: " + ffmpegErrorToString(openRet);
        return false;
    }

    const double safeRate = playbackRate > 0.0 ? playbackRate : 1.0;
    outputSampleRate_ = static_cast<int>(std::lround(48000.0 / safeRate));
    if (outputSampleRate_ <= 0) {
        outputSampleRate_ = 48000;
    }

    if (!resampler_.open(codecContext_->ch_layout, codecContext_->sample_rate, codecContext_->sample_fmt,
                         outputSampleRate_, outError)) {
        return false;
    }

    return true;
}

void FFmpegAudioDecoder::demuxLoop() {
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

        if (packet->stream_index == audioStreamIndex_) {
            packetQueue_.push(packet);
        } else {
            av_packet_free(&packet);
        }
    }

    AVPacket* flushPacket = av_packet_alloc();
    if (flushPacket != nullptr) {
        flushPacket->data = nullptr;
        flushPacket->size = 0;
        flushPacket->stream_index = audioStreamIndex_;
        packetQueue_.push(flushPacket);
    }
    demuxFinished_.store(true);
}

void FFmpegAudioDecoder::decodeLoop() {
    AVFrame* frame = av_frame_alloc();
    if (frame == nullptr) {
        return;
    }

    AVPacket* packet = nullptr;
    while (packetQueue_.waitPop(packet)) {
        if (packet == nullptr) {
            continue;
        }

        const bool flush = packet->data == nullptr && packet->size == 0;
        const int sendRet = avcodec_send_packet(codecContext_, flush ? nullptr : packet);
        av_packet_free(&packet);

        if (sendRet < 0 && sendRet != AVERROR(EAGAIN)) {
            continue;
        }

        while (running_.load()) {
            const int recvRet = avcodec_receive_frame(codecContext_, frame);
            if (recvRet == AVERROR(EAGAIN) || recvRet == AVERROR_EOF) {
                break;
            }
            if (recvRet < 0) {
                break;
            }

            playerlab::core::AudioFrame outFrame;
            if (!resampler_.resample(frame, outFrame)) {
                continue;
            }

            const AVStream* audioStream = formatContext_->streams[audioStreamIndex_];
            outFrame.ptsSec = frame->best_effort_timestamp == AV_NOPTS_VALUE
                                  ? 0.0
                                  : frame->best_effort_timestamp * av_q2d(audioStream->time_base);
            frameQueue_.push(std::move(outFrame));
        }

        if (flush) {
            break;
        }
    }

    decodeFinished_.store(true);
    av_frame_free(&frame);
}

}  // namespace playerlab::ffmpeg
