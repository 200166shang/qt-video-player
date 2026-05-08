#include "core/PlayerController.h"

#include <QMetaType>

#include "audio/QtAudioOutput.h"
#include "ffmpeg/FFmpegGlobal.h"

namespace playerlab::core {

PlayerController::PlayerController(QObject* parent) : QObject(parent) {
    playerlab::ffmpeg::FFmpegGlobal::initialize();
    qRegisterMetaType<playerlab::core::VideoFrame>("playerlab::core::VideoFrame");

    audioOutput_ = std::make_unique<playerlab::audio::QtAudioOutput>();

    framePumpTimer_.setInterval(15);
    connect(&framePumpTimer_, &QTimer::timeout, this, &PlayerController::onFramePump);
    audioPumpTimer_.setInterval(10);
    connect(&audioPumpTimer_, &QTimer::timeout, this, &PlayerController::onAudioPump);
}

PlayerController::~PlayerController() {
    stop();
}

void PlayerController::open(const QString& localFilePath) {
    stop();

    playerlab::core::MediaSource source{
        .uri = localFilePath.toStdString(),
    };

    playerlab::core::MediaInfo info;
    std::string error;
    if (!demuxer_.open(source, info, error)) {
        emit openFailed(QString::fromStdString(error));
        return;
    }

    if (!videoDecoder_.open(source, error)) {
        emit openFailed(QString::fromStdString(error));
        return;
    }

    if (info.hasAudio) {
        if (!audioDecoder_.open(source, error)) {
            videoDecoder_.stop();
            emit openFailed(QString::fromStdString(error));
            return;
        }
        if (!audioOutput_->open(48000, 2, playerlab::core::AudioSampleFormat::S16)) {
            audioDecoder_.stop();
            videoDecoder_.stop();
            emit openFailed("open audio output failed");
            return;
        }
        audioOutput_->setVolume(volume_);
        audioOutput_->setMuted(muted_);
        if (paused_) {
            audioOutput_->pause();
        }
    }

    emit mediaInfoChanged(info);
    framePumpTimer_.start();
    if (info.hasAudio) {
        audioPumpTimer_.start();
    }
}

void PlayerController::stop() {
    framePumpTimer_.stop();
    audioPumpTimer_.stop();
    videoDecoder_.stop();
    audioDecoder_.stop();
    if (audioOutput_ != nullptr) {
        audioOutput_->stop();
        audioOutput_->close();
    }
}

void PlayerController::onFramePump() {
    playerlab::core::VideoFrame frame;
    bool gotFrame = false;
    while (videoDecoder_.tryPopFrame(frame)) {
        gotFrame = true;
    }

    if (gotFrame) {
        emit videoFrameReady(frame);
    }
}

void PlayerController::onAudioPump() {
    if (paused_ || audioOutput_ == nullptr) {
        return;
    }

    audioOutput_->pump();

    playerlab::core::AudioFrame frame;
    int frameCount = 0;
    constexpr int kMaxFramesPerTick = 8;
    while (frameCount < kMaxFramesPerTick && audioDecoder_.tryPopFrame(frame)) {
        audioOutput_->write(frame);
        ++frameCount;
    }
}

void PlayerController::setVolume(const float volume) {
    volume_ = volume;
    if (audioOutput_ != nullptr) {
        audioOutput_->setVolume(volume_);
    }
}

void PlayerController::setMuted(const bool muted) {
    muted_ = muted;
    if (audioOutput_ != nullptr) {
        audioOutput_->setMuted(muted_);
    }
}

void PlayerController::setPaused(const bool paused) {
    paused_ = paused;
    if (audioOutput_ == nullptr) {
        return;
    }
    if (paused_) {
        audioOutput_->pause();
    } else {
        audioOutput_->resume();
    }
}

}  // namespace playerlab::core
