#include "ffmpeg/FFmpegResampler.h"

extern "C" {
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
}

#include <algorithm>

namespace {

constexpr int kDefaultOutputSampleRate = 48000;
constexpr AVSampleFormat kOutputSampleFormat = AV_SAMPLE_FMT_S16;

}  // namespace

namespace playerlab::ffmpeg {

FFmpegResampler::~FFmpegResampler() {
    close();
}

bool FFmpegResampler::open(const AVChannelLayout& inChannelLayout, const int inSampleRate, const int inSampleFormat,
                           const int outSampleRate, std::string& outError) {
    close();
    outSampleRate_ = outSampleRate > 0 ? outSampleRate : kDefaultOutputSampleRate;

    AVChannelLayout outChannelLayout;
    av_channel_layout_default(&outChannelLayout, 2);

    swrContext_ = swr_alloc();
    if (swrContext_ == nullptr) {
        av_channel_layout_uninit(&outChannelLayout);
        outError = "alloc swr context failed";
        return false;
    }

    av_opt_set_chlayout(swrContext_, "in_chlayout", &inChannelLayout, 0);
    av_opt_set_int(swrContext_, "in_sample_rate", inSampleRate, 0);
    av_opt_set_sample_fmt(swrContext_, "in_sample_fmt", static_cast<AVSampleFormat>(inSampleFormat), 0);

    av_opt_set_chlayout(swrContext_, "out_chlayout", &outChannelLayout, 0);
    av_opt_set_int(swrContext_, "out_sample_rate", outSampleRate_, 0);
    av_opt_set_sample_fmt(swrContext_, "out_sample_fmt", kOutputSampleFormat, 0);
    av_channel_layout_uninit(&outChannelLayout);

    const int initRet = swr_init(swrContext_);
    if (initRet < 0) {
        close();
        outError = "init swr failed";
        return false;
    }
    return true;
}

void FFmpegResampler::close() {
    if (swrContext_ != nullptr) {
        swr_free(&swrContext_);
    }
}

bool FFmpegResampler::isOpen() const {
    return swrContext_ != nullptr;
}

bool FFmpegResampler::resample(const AVFrame* frame, playerlab::core::AudioFrame& outFrame) {
    if (frame == nullptr || swrContext_ == nullptr) {
        return false;
    }

    const int outSamples = av_rescale_rnd(swr_get_delay(swrContext_, frame->sample_rate) + frame->nb_samples,
                                          outSampleRate_, frame->sample_rate, AV_ROUND_UP);
    if (outSamples <= 0) {
        return false;
    }

    constexpr int outChannels = 2;
    const int outBufferSize = av_samples_get_buffer_size(nullptr, outChannels, outSamples, kOutputSampleFormat, 1);
    if (outBufferSize <= 0) {
        return false;
    }

    outFrame.data.resize(static_cast<std::size_t>(outBufferSize));
    std::uint8_t* outData[1] = {outFrame.data.data()};
    const int converted = swr_convert(swrContext_, outData, outSamples, frame->extended_data, frame->nb_samples);
    if (converted <= 0) {
        outFrame.data.clear();
        return false;
    }

    const int bytesPerSample = av_get_bytes_per_sample(kOutputSampleFormat);
    const int usedBytes = converted * outChannels * bytesPerSample;
    outFrame.data.resize(static_cast<std::size_t>(std::max(0, usedBytes)));
    outFrame.sampleRate = outSampleRate_;
    outFrame.channels = outChannels;
    outFrame.sampleFormat = playerlab::core::AudioSampleFormat::S16;
    outFrame.sampleCount = converted;
    return !outFrame.data.empty();
}

}  // namespace playerlab::ffmpeg
