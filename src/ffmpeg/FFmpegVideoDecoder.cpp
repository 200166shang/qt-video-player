#include "ffmpeg/FFmpegVideoDecoder.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
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

bool FFmpegVideoDecoder::open(const playerlab::core::MediaSource& source, std::string& outError) {
    stop();
    packetQueue_.reset();
    frameQueue_.reset();

    if (!openInput(source.uri, outError)) {
        return false;
    }
    if (!openVideoDecoder(outError)) {
        stop();
        return false;
    }

    running_.store(true);
    demuxThread_ = std::thread([this]() { demuxLoop(); });
    decodeThread_ = std::thread([this]() { decodeLoop(); });
    return true;
}

void FFmpegVideoDecoder::stop() {
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

    if (codecContext_ != nullptr) {
        avcodec_free_context(&codecContext_);
    }
    if (formatContext_ != nullptr) {
        avformat_close_input(&formatContext_);
    }
    videoStreamIndex_ = -1;
}

bool FFmpegVideoDecoder::tryPopFrame(playerlab::core::VideoFrame& outFrame) {
    return frameQueue_.tryPop(outFrame);
}

bool FFmpegVideoDecoder::openInput(const std::string& uri, std::string& outError) {
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

    videoStreamIndex_ = av_find_best_stream(formatContext_, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (videoStreamIndex_ < 0) {
        outError = "video stream not found";
        return false;
    }

    return true;
}

bool FFmpegVideoDecoder::openVideoDecoder(std::string& outError) {
    AVStream* stream = formatContext_->streams[videoStreamIndex_];
    const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (codec == nullptr) {
        outError = "video decoder not found";
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
        outError = "open video decoder failed: " + ffmpegErrorToString(openRet);
        return false;
    }

    if (codecContext_->pix_fmt != AV_PIX_FMT_YUV420P) {
        outError = "only YUV420P is supported in iter-04";
        return false;
    }

    return true;
}

void FFmpegVideoDecoder::demuxLoop() {
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

        if (packet->stream_index == videoStreamIndex_) {
            packetQueue_.push(packet);
        } else {
            av_packet_free(&packet);
        }
    }

    AVPacket* flushPacket = av_packet_alloc();
    if (flushPacket != nullptr) {
        flushPacket->data = nullptr;
        flushPacket->size = 0;
        flushPacket->stream_index = videoStreamIndex_;
        packetQueue_.push(flushPacket);
    }
}

void FFmpegVideoDecoder::decodeLoop() {
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

            const AVStream* videoStream = formatContext_->streams[videoStreamIndex_];
            const double ptsSec = frame->best_effort_timestamp == AV_NOPTS_VALUE
                                      ? 0.0
                                      : frame->best_effort_timestamp * av_q2d(videoStream->time_base);
            frameQueue_.push(toVideoFrame(frame, ptsSec));
        }

        if (flush) {
            break;
        }
    }

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
