#pragma once

#include <atomic>
#include <string>
#include <thread>

#include "core/MediaInfo.h"
#include "core/MediaSource.h"
#include "ffmpeg/PacketQueue.h"

struct AVCodecParameters;
struct AVFormatContext;
struct AVPacket;
struct AVRational;

namespace playerlab::ffmpeg {

class FFmpegReadWorker {
public:
    FFmpegReadWorker() = default;
    ~FFmpegReadWorker();

    FFmpegReadWorker(const FFmpegReadWorker&) = delete;
    FFmpegReadWorker& operator=(const FFmpegReadWorker&) = delete;

    bool open(const playerlab::core::MediaSource& source, playerlab::core::MediaInfo& outInfo, std::string& outError,
              double startPositionSec = 0.0);
    void start(PacketQueue<AVPacket*>* videoPacketQueue, PacketQueue<AVPacket*>* audioPacketQueue);
    void stop();

    [[nodiscard]] bool isFinished() const { return readFinished_.load(); }
    [[nodiscard]] int videoStreamIndex() const { return videoStreamIndex_; }
    [[nodiscard]] int audioStreamIndex() const { return audioStreamIndex_; }
    [[nodiscard]] const AVCodecParameters* videoCodecParameters() const;
    [[nodiscard]] const AVCodecParameters* audioCodecParameters() const;
    [[nodiscard]] AVRational videoTimeBase() const;
    [[nodiscard]] AVRational audioTimeBase() const;

private:
    bool seekTo(double startPositionSec, std::string& outError);
    void readLoop();
    static playerlab::core::MediaInfo buildMediaInfo(const playerlab::core::MediaSource& source,
                                                     const AVFormatContext* formatContext);
    static AVPacket* makeFlushPacket(int streamIndex);

    std::atomic<bool> running_{false};
    std::atomic<bool> readFinished_{false};
    AVFormatContext* formatContext_ = nullptr;
    int videoStreamIndex_ = -1;
    int audioStreamIndex_ = -1;
    PacketQueue<AVPacket*>* videoPacketQueue_ = nullptr;
    PacketQueue<AVPacket*>* audioPacketQueue_ = nullptr;
    std::thread readThread_;
};

}  // namespace playerlab::ffmpeg
