#pragma once

#include <atomic>
#include <string>
#include <thread>

#include "core/AudioFrame.h"
#include "core/MediaSource.h"
#include "ffmpeg/FFmpegResampler.h"
#include "ffmpeg/FrameQueue.h"
#include "ffmpeg/PacketQueue.h"

struct AVCodecContext;
struct AVFormatContext;
struct AVPacket;

namespace playerlab::ffmpeg {

class FFmpegAudioDecoder {
public:
    FFmpegAudioDecoder() = default;
    ~FFmpegAudioDecoder();

    FFmpegAudioDecoder(const FFmpegAudioDecoder&) = delete;
    FFmpegAudioDecoder& operator=(const FFmpegAudioDecoder&) = delete;

    bool open(const playerlab::core::MediaSource& source, std::string& outError, double startPositionSec = 0.0,
              double playbackRate = 1.0);
    void stop();
    [[nodiscard]] bool tryPopFrame(playerlab::core::AudioFrame& outFrame);
    [[nodiscard]] bool isDrained() const;

private:
    bool openInput(const std::string& uri, std::string& outError, double startPositionSec);
    bool openAudioDecoder(std::string& outError, double playbackRate);
    void demuxLoop();
    void decodeLoop();

    std::atomic<bool> running_{false};
    AVFormatContext* formatContext_ = nullptr;
    AVCodecContext* codecContext_ = nullptr;
    int audioStreamIndex_ = -1;

    FFmpegResampler resampler_;
    PacketQueue<AVPacket*> packetQueue_;
    FrameQueue<playerlab::core::AudioFrame> frameQueue_;

    std::thread demuxThread_;
    std::thread decodeThread_;
    std::atomic<bool> demuxFinished_{false};
    std::atomic<bool> decodeFinished_{false};
    int outputSampleRate_ = 48000;
};

}  // namespace playerlab::ffmpeg
