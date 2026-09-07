#include "ffmpeg/FFmpegAudioDecoder.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}

#include <cmath>

#include "utils/Logger.h"

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
    eofSerial_.store(-1);
    timeBase_ = timeBase;

    if (!openAudioDecoder(codecParameters, outError, playbackRate)) {
        stop();
        return false;
    }

    return true;
}

void FFmpegAudioDecoder::start(PacketQueue<QueuedPacket>* packetQueue) {
    packetQueue_ = packetQueue;
    eofSerial_.store(-1);
    running_.store(true);
    if (playerlab::utils::Logger::isPipelineDebugEnabled()) {
        LOG_DEBUG("Audio decoder start: outputSampleRate={}", outputSampleRate_);
    }
    decodeThread_ = std::thread([this]() { decodeLoop(); });
}

void FFmpegAudioDecoder::stop() {
    running_.store(false);
    if (packetQueue_ != nullptr) {
        packetQueue_->abort();
    }
    frameQueue_.abort();

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
    eofSerial_.store(-1);
    if (playerlab::utils::Logger::isPipelineDebugEnabled()) {
        LOG_DEBUG("Audio decoder stop");
    }
}

bool FFmpegAudioDecoder::tryPopFrame(playerlab::core::AudioFrame& outFrame) {
    return frameQueue_.tryPop(outFrame);
}

bool FFmpegAudioDecoder::isDrained(const int serial) const {
    return eofSerial_.load() == serial && frameQueue_.empty();
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

    codecContext_->pkt_timebase = timeBase_;

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
        return;
    }

    std::uint64_t decodedFrames = 0;
    std::uint64_t flushCount = 0;
    std::uint64_t eofCount = 0;

    AVFrame* frame = av_frame_alloc();
    if (frame == nullptr) {
        return;
    }

    QueuedPacket queuedPacket;
    int decoderSerial = 0;
    while (packetQueue_->waitPop(queuedPacket)) {
        if (queuedPacket.kind == QueuedPacketKind::Flush) {
            avcodec_flush_buffers(codecContext_);
            decoderSerial = queuedPacket.serial;
            eofSerial_.store(-1);
            ++flushCount;
            continue;
        }

        if (queuedPacket.kind == QueuedPacketKind::Eof) {
            decoderSerial = queuedPacket.serial;
            const int sendRet = avcodec_send_packet(codecContext_, nullptr);
            if (sendRet < 0 && sendRet != AVERROR(EAGAIN) && sendRet != AVERROR_EOF) {
                eofSerial_.store(decoderSerial);
                ++eofCount;
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
                outFrame.serial = decoderSerial;
                if (!frameQueue_.push(std::move(outFrame))) {
                    break;
                }
                ++decodedFrames;
            }

            eofSerial_.store(decoderSerial);
            ++eofCount;
            continue;
        }

        AVPacket* packet = queuedPacket.packet;
        if (packet == nullptr) {
            continue;
        }

        decoderSerial = queuedPacket.serial;
        const int sendRet = avcodec_send_packet(codecContext_, packet);
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
            outFrame.serial = decoderSerial;
            if (!frameQueue_.push(std::move(outFrame))) {
                break;
            }
            ++decodedFrames;
        }
    }

    av_frame_free(&frame);
    if (playerlab::utils::Logger::isPipelineDebugEnabled()) {
        LOG_DEBUG("Audio decoder exit: frames={} flushes={} eofs={} packetQ={} frameQ={}", decodedFrames, flushCount,
                  eofCount, packetQueue_ != nullptr ? packetQueue_->size() : 0, frameQueue_.size());
    }
}

}  // namespace playerlab::ffmpeg
