#pragma once

#include <atomic>
#include <string>
#include <thread>

#include "core/MediaSource.h"
#include "core/VideoFrame.h"
#include "ffmpeg/FrameQueue.h"
#include "ffmpeg/PacketQueue.h"

struct AVCodecContext;
struct AVFormatContext;
struct AVFrame;
struct AVPacket;

namespace playerlab::ffmpeg {

class FFmpegVideoDecoder {
public:
    FFmpegVideoDecoder() = default;
    ~FFmpegVideoDecoder();

    FFmpegVideoDecoder(const FFmpegVideoDecoder&) = delete;
    FFmpegVideoDecoder& operator=(const FFmpegVideoDecoder&) = delete;

    bool open(const playerlab::core::MediaSource& source, std::string& outError);
    void stop();

    [[nodiscard]] bool tryPopFrame(playerlab::core::VideoFrame& outFrame);

private:
    bool openInput(const std::string& uri, std::string& outError);
    bool openVideoDecoder(std::string& outError);
    void demuxLoop();
    void decodeLoop();
    static playerlab::core::VideoFrame toVideoFrame(const AVFrame* frame, double ptsSec);

    std::atomic<bool> running_{false};
    AVFormatContext* formatContext_ = nullptr;
    AVCodecContext* codecContext_ = nullptr;
    int videoStreamIndex_ = -1;

    PacketQueue<AVPacket*> packetQueue_;
    FrameQueue<playerlab::core::VideoFrame> frameQueue_;

    std::thread demuxThread_;
    std::thread decodeThread_;
};

}  // namespace playerlab::ffmpeg
