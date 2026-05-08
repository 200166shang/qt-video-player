#pragma once

#include "core/AudioFrame.h"

namespace playerlab::audio {

class IAudioOutput {
public:
    virtual ~IAudioOutput() = default;

    virtual bool open(int sampleRate, int channels, playerlab::core::AudioSampleFormat sampleFormat) = 0;
    virtual void close() = 0;
    virtual void write(const playerlab::core::AudioFrame& frame) = 0;
    virtual void pump() = 0;
    virtual void setVolume(float volume) = 0;
    virtual void setMuted(bool muted) = 0;
    virtual void pause() = 0;
    virtual void resume() = 0;
    virtual void stop() = 0;
};

}  // namespace playerlab::audio
