#pragma once

#include <atomic>
#include <string>
#include <thread>

extern "C" {
#include <libavutil/rational.h>
}

#include "core/AudioFrame.h"
#include "ffmpeg/FFmpegResampler.h"
#include "ffmpeg/FrameQueue.h"
#include "ffmpeg/PacketQueue.h"

struct AVCodecParameters;
struct AVCodecContext;
struct AVPacket;

namespace playerlab::ffmpeg {

class FFmpegAudioDecoder {
public:
    FFmpegAudioDecoder() = default;
    ~FFmpegAudioDecoder();

    FFmpegAudioDecoder(const FFmpegAudioDecoder&) = delete;
    FFmpegAudioDecoder& operator=(const FFmpegAudioDecoder&) = delete;

    bool open(const AVCodecParameters* codecParameters, AVRational timeBase, std::string& outError,
              double playbackRate = 1.0);
    void start(PacketQueue<AVPacket*>* packetQueue);
    void stop();
    [[nodiscard]] bool tryPopFrame(playerlab::core::AudioFrame& outFrame);
    [[nodiscard]] bool isDrained() const;

private:
    bool openAudioDecoder(const AVCodecParameters* codecParameters, std::string& outError, double playbackRate);
    void decodeLoop();

    std::atomic<bool> running_{false};
    AVCodecContext* codecContext_ = nullptr;
    AVRational timeBase_{0, 1};

    FFmpegResampler resampler_;
    PacketQueue<AVPacket*>* packetQueue_ = nullptr;
    FrameQueue<playerlab::core::AudioFrame> frameQueue_;

    std::thread decodeThread_;
    std::atomic<bool> decodeFinished_{false};
    int outputSampleRate_ = 48000;
};

}  // namespace playerlab::ffmpeg
