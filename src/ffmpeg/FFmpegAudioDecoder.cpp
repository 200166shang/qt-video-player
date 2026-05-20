#include "ffmpeg/FFmpegAudioDecoder.h"

extern "C" {
#include <libavcodec/avcodec.h>
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

bool FFmpegAudioDecoder::open(const AVCodecParameters* codecParameters, const AVRational timeBase,
                              std::string& outError, const double playbackRate) {
    stop();
    frameQueue_.reset();
    resampler_.close();
    decodeFinished_.store(false);
    timeBase_ = timeBase;

    if (!openAudioDecoder(codecParameters, outError, playbackRate)) {
        stop();
        return false;
    }

    return true;
}

void FFmpegAudioDecoder::start(PacketQueue<AVPacket*>* packetQueue) {
    packetQueue_ = packetQueue;
    decodeFinished_.store(false);
    running_.store(true);
    decodeThread_ = std::thread([this]() { decodeLoop(); });
}

void FFmpegAudioDecoder::stop() {
    running_.store(false);
    if (packetQueue_ != nullptr) {
        packetQueue_->abort();
    }

    if (decodeThread_.joinable()) {
        decodeThread_.join();
    }

    frameQueue_.clear();
    resampler_.close();

    if (codecContext_ != nullptr) {
        avcodec_free_context(&codecContext_);
    }
    packetQueue_ = nullptr;
    timeBase_ = AVRational{0, 1};
    decodeFinished_.store(false);
}

bool FFmpegAudioDecoder::tryPopFrame(playerlab::core::AudioFrame& outFrame) {
    return frameQueue_.tryPop(outFrame);
}

bool FFmpegAudioDecoder::isDrained() const {
    return decodeFinished_.load() && (packetQueue_ == nullptr || packetQueue_->empty()) && frameQueue_.empty();
}

bool FFmpegAudioDecoder::openAudioDecoder(const AVCodecParameters* codecParameters, std::string& outError,
                                          const double playbackRate) {
    if (codecParameters == nullptr) {
        outError = "audio codec parameters missing";
        return false;
    }

    const AVCodec* codec = avcodec_find_decoder(codecParameters->codec_id);
    if (codec == nullptr) {
        outError = "audio decoder not found";
        return false;
    }

    codecContext_ = avcodec_alloc_context3(codec);
    if (codecContext_ == nullptr) {
        outError = "alloc codec context failed";
        return false;
    }

    const int parRet = avcodec_parameters_to_context(codecContext_, codecParameters);
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

void FFmpegAudioDecoder::decodeLoop() {
    if (packetQueue_ == nullptr || codecContext_ == nullptr) {
        decodeFinished_.store(true);
        return;
    }

    AVFrame* frame = av_frame_alloc();
    if (frame == nullptr) {
        decodeFinished_.store(true);
        return;
    }

    AVPacket* packet = nullptr;
    while (packetQueue_->waitPop(packet)) {
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

            outFrame.ptsSec = frame->best_effort_timestamp == AV_NOPTS_VALUE
                                  ? 0.0
                                  : frame->best_effort_timestamp * av_q2d(timeBase_);
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
