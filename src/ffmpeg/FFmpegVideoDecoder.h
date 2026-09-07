#pragma once

#include <atomic>
#include <string>
#include <thread>

extern "C" {
#include <libavutil/rational.h>
}

#include "core/VideoFrame.h"
#include "ffmpeg/CircularFrameQueue.h"
#include "ffmpeg/PacketQueue.h"
#include "ffmpeg/QueuedPacket.h"

struct AVCodecParameters;
struct AVCodecContext;
struct AVFrame;

namespace playerlab::ffmpeg {

class FFmpegVideoDecoder {
public:
    FFmpegVideoDecoder() = default;
    ~FFmpegVideoDecoder();

    FFmpegVideoDecoder(const FFmpegVideoDecoder&) = delete;
    FFmpegVideoDecoder& operator=(const FFmpegVideoDecoder&) = delete;

    bool open(const AVCodecParameters* codecParameters, AVRational timeBase, std::string& outError);
    void start(PacketQueue<QueuedPacket>* packetQueue);
    void stop();

    [[nodiscard]] bool tryPopFrame(playerlab::core::VideoFrame& outFrame);
    [[nodiscard]] bool isDrained(int serial) const;

private:
    bool openVideoDecoder(const AVCodecParameters* codecParameters, std::string& outError);
    void decodeLoop();
    static playerlab::core::VideoFrame toVideoFrame(const AVFrame* frame, double ptsSec, int serial);

    std::atomic<bool> running_{false};
    AVCodecContext* codecContext_ = nullptr;
    AVRational timeBase_{0, 1};

    PacketQueue<QueuedPacket>* packetQueue_ = nullptr;
    CircularFrameQueue<playerlab::core::VideoFrame, 24> frameQueue_;

    std::thread decodeThread_;
    std::atomic<int> eofSerial_{-1};
};

}  // namespace playerlab::ffmpeg
