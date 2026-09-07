#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

#include "core/MediaInfo.h"
#include "core/MediaSource.h"
#include "ffmpeg/PacketQueue.h"
#include "ffmpeg/QueuedPacket.h"

struct AVCodecParameters;
struct AVFormatContext;
struct AVRational;

namespace playerlab::ffmpeg {

struct SeekRequest {
    double targetSec = 0.0;
    double relSec = 0.0;
    int flags = 0;
};

class FFmpegReadWorker {
public:
    FFmpegReadWorker() = default;
    ~FFmpegReadWorker();

    FFmpegReadWorker(const FFmpegReadWorker&) = delete;
    FFmpegReadWorker& operator=(const FFmpegReadWorker&) = delete;

    bool open(const playerlab::core::MediaSource& source, playerlab::core::MediaInfo& outInfo, std::string& outError,
              double startPositionSec = 0.0);
    void start(PacketQueue<QueuedPacket>* videoPacketQueue, PacketQueue<QueuedPacket>* audioPacketQueue);
    void stop();
    // Returns the serial reserved for this request. Coalesced requests share
    // the same serial and the read thread applies only the latest target.
    int requestSeek(const SeekRequest& request);

    [[nodiscard]] bool isFinished() const { return readFinished_.load(); }
    [[nodiscard]] int currentSerial() const { return currentSerial_.load(); }
    [[nodiscard]] bool isEofSerial(int serial) const { return eofSerial_.load() == serial; }
    [[nodiscard]] int videoStreamIndex() const { return videoStreamIndex_; }
    [[nodiscard]] int audioStreamIndex() const { return audioStreamIndex_; }
    [[nodiscard]] const AVCodecParameters* videoCodecParameters() const;
    [[nodiscard]] const AVCodecParameters* audioCodecParameters() const;
    [[nodiscard]] AVRational videoTimeBase() const;
    [[nodiscard]] AVRational audioTimeBase() const;

private:
    bool seekTo(double startPositionSec, std::string& outError);
    bool performPendingSeek();
    void waitForSeekRequest();
    void readLoop();
    static playerlab::core::MediaInfo buildMediaInfo(const playerlab::core::MediaSource& source,
                                                     const AVFormatContext* formatContext);
    static void freeQueuedPacket(QueuedPacket& queuedPacket);

    std::atomic<bool> running_{false};
    std::atomic<bool> readFinished_{false};
    std::atomic<int> currentSerial_{1};
    std::atomic<int> eofSerial_{-1};
    AVFormatContext* formatContext_ = nullptr;
    int videoStreamIndex_ = -1;
    int audioStreamIndex_ = -1;
    PacketQueue<QueuedPacket>* videoPacketQueue_ = nullptr;
    PacketQueue<QueuedPacket>* audioPacketQueue_ = nullptr;
    std::thread readThread_;
    std::mutex seekMutex_;
    std::condition_variable seekCv_;
    std::optional<SeekRequest> pendingSeekRequest_;
    int pendingSeekSerial_ = 0;
};

}  // namespace playerlab::ffmpeg
