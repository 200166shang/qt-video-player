#pragma once

#include <atomic>
#include <string>
#include <thread>

extern "C" {
#include <libavutil/rational.h>
}

#include "core/VideoFrame.h"
#include "ffmpeg/FrameQueue.h"
#include "ffmpeg/PacketQueue.h"

struct AVCodecParameters;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;

namespace playerlab::ffmpeg {

class FFmpegVideoDecoder {
public:
    FFmpegVideoDecoder() = default;
    ~FFmpegVideoDecoder();

    FFmpegVideoDecoder(const FFmpegVideoDecoder&) = delete;
    FFmpegVideoDecoder& operator=(const FFmpegVideoDecoder&) = delete;

    bool open(const AVCodecParameters* codecParameters, AVRational timeBase, std::string& outError);
    void start(PacketQueue<AVPacket*>* packetQueue);
    void stop();

    [[nodiscard]] bool tryPopFrame(playerlab::core::VideoFrame& outFrame);
    [[nodiscard]] bool isDrained() const;

private:
    bool openVideoDecoder(const AVCodecParameters* codecParameters, std::string& outError);
    void decodeLoop();
    static playerlab::core::VideoFrame toVideoFrame(const AVFrame* frame, double ptsSec);

    std::atomic<bool> running_{false};
    AVCodecContext* codecContext_ = nullptr;
    AVRational timeBase_{0, 1};

    PacketQueue<AVPacket*>* packetQueue_ = nullptr;
    FrameQueue<playerlab::core::VideoFrame> frameQueue_;

    std::thread decodeThread_;
    std::atomic<bool> decodeFinished_{false};
};

}  // namespace playerlab::ffmpeg
