#include "ffmpeg/FFmpegVideoDecoder.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}

#include <algorithm>

#include "utils/Logger.h"

namespace {

std::string ffmpegErrorToString(const int errNum) {
    char errBuf[AV_ERROR_MAX_STRING_SIZE] = {0};
    av_strerror(errNum, errBuf, sizeof(errBuf));
    return std::string(errBuf);
}

}  // namespace

namespace playerlab::ffmpeg {

FFmpegVideoDecoder::~FFmpegVideoDecoder() {
    stop();
}

bool FFmpegVideoDecoder::open(const AVCodecParameters* codecParameters, const AVRational timeBase,
                              std::string& outError) {
    stop();
    frameQueue_.reset();
    eofSerial_.store(-1);
    timeBase_ = timeBase;

    if (!openVideoDecoder(codecParameters, outError)) {
        stop();
        return false;
    }

    return true;
}

void FFmpegVideoDecoder::start(PacketQueue<QueuedPacket>* packetQueue) {
    packetQueue_ = packetQueue;
    eofSerial_.store(-1);
    running_.store(true);
    if (playerlab::utils::Logger::isPipelineDebugEnabled()) {
        LOG_DEBUG("Video decoder start");
    }
    decodeThread_ = std::thread([this]() { decodeLoop(); });
}

void FFmpegVideoDecoder::stop() {
    running_.store(false);
    if (packetQueue_ != nullptr) {
        packetQueue_->abort();
    }
    frameQueue_.abort();

    if (decodeThread_.joinable()) {
        decodeThread_.join();
    }

    frameQueue_.clear();

    if (codecContext_ != nullptr) {
        avcodec_free_context(&codecContext_);
    }
    packetQueue_ = nullptr;
    timeBase_ = AVRational{0, 1};
    eofSerial_.store(-1);
    if (playerlab::utils::Logger::isPipelineDebugEnabled()) {
        LOG_DEBUG("Video decoder stop");
    }
}

bool FFmpegVideoDecoder::tryPopFrame(playerlab::core::VideoFrame& outFrame) {
    return frameQueue_.tryPop(outFrame);
}

bool FFmpegVideoDecoder::isDrained(const int serial) const {
    return eofSerial_.load() == serial && frameQueue_.empty();
}

bool FFmpegVideoDecoder::openVideoDecoder(const AVCodecParameters* codecParameters, std::string& outError) {
    if (codecParameters == nullptr) {
        outError = "video codec parameters missing";
        return false;
    }

    const AVCodec* codec = avcodec_find_decoder(codecParameters->codec_id);
    if (codec == nullptr) {
        outError = "video decoder not found";
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

    // Keep decoded frame timestamps in the stream time base. Without this,
    // codecs such as H.264 may expose best_effort_timestamp in an undefined
    // unit, which breaks playback timing and seek validation.
    codecContext_->pkt_timebase = timeBase_;

    const int openRet = avcodec_open2(codecContext_, codec, nullptr);
    if (openRet < 0) {
        outError = "open video decoder failed: " + ffmpegErrorToString(openRet);
        return false;
    }

    // For codecs such as H.264, codecContext_->pix_fmt is not negotiated until
    // the first frame is decoded. Validate the demuxer's advertised format
    // here instead of rejecting every otherwise supported YUV420P stream.
    if (codecParameters->format != AV_PIX_FMT_YUV420P) {
        outError = "only YUV420P is supported in iter-04";
        return false;
    }

    return true;
}

void FFmpegVideoDecoder::decodeLoop() {
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

                const double ptsSec = frame->best_effort_timestamp == AV_NOPTS_VALUE
                                          ? 0.0
                                          : frame->best_effort_timestamp * av_q2d(timeBase_);
                if (!frameQueue_.push(toVideoFrame(frame, ptsSec, decoderSerial))) {
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

            const double ptsSec = frame->best_effort_timestamp == AV_NOPTS_VALUE
                                      ? 0.0
                                      : frame->best_effort_timestamp * av_q2d(timeBase_);
            if (!frameQueue_.push(toVideoFrame(frame, ptsSec, decoderSerial))) {
                break;
            }
            ++decodedFrames;
        }
    }

    av_frame_free(&frame);
    if (playerlab::utils::Logger::isPipelineDebugEnabled()) {
        LOG_DEBUG("Video decoder exit: frames={} flushes={} eofs={} packetQ={} frameQ={}", decodedFrames, flushCount,
                  eofCount, packetQueue_ != nullptr ? packetQueue_->size() : 0, frameQueue_.size());
    }
}

playerlab::core::VideoFrame FFmpegVideoDecoder::toVideoFrame(const AVFrame* frame, const double ptsSec,
                                                             const int serial) {
    playerlab::core::VideoFrame out;
    out.width = frame->width;
    out.height = frame->height;
    out.ptsSec = ptsSec;
    out.serial = serial;

    const int yStride = frame->linesize[0];
    const int uStride = frame->linesize[1];
    const int vStride = frame->linesize[2];

    out.linesize = {yStride, uStride, vStride};
    out.planes[0].resize(static_cast<std::size_t>(yStride * frame->height));
    out.planes[1].resize(static_cast<std::size_t>(uStride * (frame->height / 2)));
    out.planes[2].resize(static_cast<std::size_t>(vStride * (frame->height / 2)));

    for (int row = 0; row < frame->height; ++row) {
        std::copy_n(frame->data[0] + row * yStride, yStride, out.planes[0].data() + row * yStride);
    }
    for (int row = 0; row < frame->height / 2; ++row) {
        std::copy_n(frame->data[1] + row * uStride, uStride, out.planes[1].data() + row * uStride);
        std::copy_n(frame->data[2] + row * vStride, vStride, out.planes[2].data() + row * vStride);
    }

    return out;
}

}  // namespace playerlab::ffmpeg
