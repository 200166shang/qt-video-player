#include "audio/QtAudioOutput.h"

#include <algorithm>

#include <QAudioDevice>
#include <QAudioSink>
#include <QMediaDevices>

namespace playerlab::audio {

QtAudioOutput::~QtAudioOutput() {
    close();
}

bool QtAudioOutput::open(const int sampleRate, const int channels,
                         const playerlab::core::AudioSampleFormat sampleFormat) {
    close();

    QAudioFormat format;
    format.setSampleRate(sampleRate);
    format.setChannelCount(channels);
    format.setSampleFormat(sampleFormat == playerlab::core::AudioSampleFormat::Float32
                               ? QAudioFormat::SampleFormat::Float
                               : QAudioFormat::SampleFormat::Int16);

    const QAudioDevice device = QMediaDevices::defaultAudioOutput();
    if (!device.isFormatSupported(format)) {
        return false;
    }

    sink_ = std::make_unique<QAudioSink>(device, format);
    sink_->setVolume(muted_ ? 0.0F : volume_);
    ioDevice_ = sink_->start();
    if (ioDevice_ == nullptr) {
        sink_.reset();
        return false;
    }

    format_ = format;
    return true;
}

void QtAudioOutput::close() {
    if (sink_ != nullptr) {
        sink_->stop();
    }
    ioDevice_ = nullptr;
    sink_.reset();
    pendingData_.clear();
    pendingOffset_ = 0;
}

void QtAudioOutput::write(const playerlab::core::AudioFrame& frame) {
    if (sink_ == nullptr || ioDevice_ == nullptr || !frame.isValid()) {
        return;
    }

    flushPending();
    pendingData_.insert(pendingData_.end(), frame.data.begin(), frame.data.end());
    flushPending();
}

void QtAudioOutput::pump() {
    flushPending();
}

void QtAudioOutput::setVolume(const float volume) {
    volume_ = std::clamp(volume, 0.0F, 1.0F);
    if (sink_ != nullptr) {
        sink_->setVolume(muted_ ? 0.0F : volume_);
    }
}

void QtAudioOutput::setMuted(const bool muted) {
    muted_ = muted;
    if (sink_ != nullptr) {
        sink_->setVolume(muted_ ? 0.0F : volume_);
    }
}

void QtAudioOutput::pause() {
    if (sink_ != nullptr) {
        sink_->suspend();
    }
}

void QtAudioOutput::resume() {
    if (sink_ != nullptr) {
        sink_->resume();
        flushPending();
    }
}

void QtAudioOutput::stop() {
    if (sink_ != nullptr) {
        sink_->stop();
        ioDevice_ = sink_->start();
    }
    pendingData_.clear();
    pendingOffset_ = 0;
}

void QtAudioOutput::flushPending() {
    if (sink_ == nullptr || ioDevice_ == nullptr || pendingOffset_ >= pendingData_.size()) {
        return;
    }

    while (pendingOffset_ < pendingData_.size()) {
        const qint64 freeBytes = sink_->bytesFree();
        if (freeBytes <= 0) {
            break;
        }

        const qint64 remaining = static_cast<qint64>(pendingData_.size() - pendingOffset_);
        const qint64 bytesToWrite = std::min(remaining, freeBytes);
        const char* ptr = reinterpret_cast<const char*>(pendingData_.data() + pendingOffset_);
        const qint64 written = ioDevice_->write(ptr, bytesToWrite);
        if (written <= 0) {
            break;
        }
        pendingOffset_ += static_cast<std::size_t>(written);
    }

    if (pendingOffset_ >= pendingData_.size()) {
        pendingData_.clear();
        pendingOffset_ = 0;
    }
}

}  // namespace playerlab::audio
