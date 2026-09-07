#pragma once

#include <atomic>
#include <string>
#include <thread>

extern "C" {
#include <libavutil/rational.h>
}

#include "core/AudioFrame.h"
#include "ffmpeg/CircularFrameQueue.h"
#include "ffmpeg/FFmpegResampler.h"
#include "ffmpeg/PacketQueue.h"
#include "ffmpeg/QueuedPacket.h"

struct AVCodecParameters;
struct AVCodecContext;

namespace playerlab::ffmpeg {

class FFmpegAudioDecoder {
public:
    FFmpegAudioDecoder() = default;
    ~FFmpegAudioDecoder();

    FFmpegAudioDecoder(const FFmpegAudioDecoder&) = delete;
    FFmpegAudioDecoder& operator=(const FFmpegAudioDecoder&) = delete;

    bool open(const AVCodecParameters* codecParameters, AVRational timeBase, std::string& outError,
              double playbackRate = 1.0);
    void start(PacketQueue<QueuedPacket>* packetQueue);
    void stop();
    [[nodiscard]] bool tryPopFrame(playerlab::core::AudioFrame& outFrame);
    [[nodiscard]] bool isDrained(int serial) const;

private:
    bool openAudioDecoder(const AVCodecParameters* codecParameters, std::string& outError, double playbackRate);
    void decodeLoop();

    std::atomic<bool> running_{false};
    AVCodecContext* codecContext_ = nullptr;
    AVRational timeBase_{0, 1};

    FFmpegResampler resampler_;
    PacketQueue<QueuedPacket>* packetQueue_ = nullptr;
    CircularFrameQueue<playerlab::core::AudioFrame, 64> frameQueue_;

    std::thread decodeThread_;
    std::atomic<int> eofSerial_{-1};
    int outputSampleRate_ = 48000;
};

}  // namespace playerlab::ffmpeg
