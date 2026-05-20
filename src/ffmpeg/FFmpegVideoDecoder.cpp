#include "ffmpeg/FFmpegVideoDecoder.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}

#include <algorithm>
#include <array>

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
    decodeFinished_.store(false);
    timeBase_ = timeBase;

    if (!openVideoDecoder(codecParameters, outError)) {
        stop();
        return false;
    }

    return true;
}

void FFmpegVideoDecoder::start(PacketQueue<AVPacket*>* packetQueue) {
    packetQueue_ = packetQueue;
    decodeFinished_.store(false);
    running_.store(true);
    decodeThread_ = std::thread([this]() { decodeLoop(); });
}

void FFmpegVideoDecoder::stop() {
    running_.store(false);
    if (packetQueue_ != nullptr) {
        packetQueue_->abort();
    }

    if (decodeThread_.joinable()) {
        decodeThread_.join();
    }

    frameQueue_.clear();

    if (codecContext_ != nullptr) {
        avcodec_free_context(&codecContext_);
    }
    packetQueue_ = nullptr;
    timeBase_ = AVRational{0, 1};
    decodeFinished_.store(false);
}

bool FFmpegVideoDecoder::tryPopFrame(playerlab::core::VideoFrame& outFrame) {
    return frameQueue_.tryPop(outFrame);
}

bool FFmpegVideoDecoder::isDrained() const {
    return decodeFinished_.load() && (packetQueue_ == nullptr || packetQueue_->empty()) && frameQueue_.empty();
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

    const int openRet = avcodec_open2(codecContext_, codec, nullptr);
    if (openRet < 0) {
        outError = "open video decoder failed: " + ffmpegErrorToString(openRet);
        return false;
    }

    if (codecContext_->pix_fmt != AV_PIX_FMT_YUV420P) {
        outError = "only YUV420P is supported in iter-04";
        return false;
    }

    return true;
}

void FFmpegVideoDecoder::decodeLoop() {
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

            const double ptsSec = frame->best_effort_timestamp == AV_NOPTS_VALUE
                                      ? 0.0
                                      : frame->best_effort_timestamp * av_q2d(timeBase_);
            frameQueue_.push(toVideoFrame(frame, ptsSec));
        }

        if (flush) {
            break;
        }
    }

    decodeFinished_.store(true);
    av_frame_free(&frame);
}

playerlab::core::VideoFrame FFmpegVideoDecoder::toVideoFrame(const AVFrame* frame, const double ptsSec) {
    playerlab::core::VideoFrame out;
    out.width = frame->width;
    out.height = frame->height;
    out.ptsSec = ptsSec;

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
