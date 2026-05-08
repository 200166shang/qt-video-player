#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include <QAudioFormat>
#include <QAudioSink>

#include "audio/IAudioOutput.h"

class QIODevice;

namespace playerlab::audio {

class QtAudioOutput : public IAudioOutput {
public:
    QtAudioOutput() = default;
    ~QtAudioOutput() override;

    bool open(int sampleRate, int channels, playerlab::core::AudioSampleFormat sampleFormat) override;
    void close() override;
    void write(const playerlab::core::AudioFrame& frame) override;
    void pump() override;
    void setVolume(float volume) override;
    void setMuted(bool muted) override;
    void pause() override;
    void resume() override;
    void stop() override;

private:
    void flushPending();

    std::unique_ptr<QAudioSink> sink_;
    QIODevice* ioDevice_ = nullptr;
    QAudioFormat format_;
    std::vector<std::uint8_t> pendingData_;
    std::size_t pendingOffset_ = 0;
    float volume_ = 1.0F;
    bool muted_ = false;
};

}  // namespace playerlab::audio
