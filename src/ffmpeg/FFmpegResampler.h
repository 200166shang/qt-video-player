#pragma once

#include <string>

#include "core/AudioFrame.h"

struct AVChannelLayout;
struct AVFrame;
struct SwrContext;

namespace playerlab::ffmpeg {

class FFmpegResampler {
public:
    FFmpegResampler() = default;
    ~FFmpegResampler();

    FFmpegResampler(const FFmpegResampler&) = delete;
    FFmpegResampler& operator=(const FFmpegResampler&) = delete;

    bool open(const AVChannelLayout& inChannelLayout, int inSampleRate, int inSampleFormat, std::string& outError);
    void close();
    [[nodiscard]] bool isOpen() const;

    bool resample(const AVFrame* frame, playerlab::core::AudioFrame& outFrame);

private:
    SwrContext* swrContext_ = nullptr;
};

}  // namespace playerlab::ffmpeg
