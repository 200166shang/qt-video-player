#include "core/PlayerController.h"

#include <QMetaObject>

#include "core/PlayerCore.h"
#include "ffmpeg/FFmpegGlobal.h"

namespace playerlab::core {

PlayerController::PlayerController(QObject* parent) : QObject(parent) {
    playerlab::ffmpeg::FFmpegGlobal::initialize();
    qRegisterMetaType<playerlab::core::VideoFrame>("playerlab::core::VideoFrame");
    qRegisterMetaType<playerlab::core::PlaybackState>("playerlab::core::PlaybackState");

    coreThread_ = new QThread(this);
    core_ = new PlayerCore();
    core_->moveToThread(coreThread_);

    connect(coreThread_, &QThread::finished, core_, &QObject::deleteLater);

    connect(core_, &PlayerCore::mediaInfoChanged, this, &PlayerController::mediaInfoChanged);
    connect(core_, &PlayerCore::videoFrameReady, this, &PlayerController::videoFrameReady);
    connect(core_, &PlayerCore::openFailed, this, &PlayerController::openFailed);
    connect(core_, &PlayerCore::playbackStateChanged, this, [this](const PlaybackState state) {
        playbackState_ = state;
        emit playbackStateChanged(state);
    });
    connect(core_, &PlayerCore::positionChanged, this, &PlayerController::positionChanged);
    connect(core_, &PlayerCore::durationChanged, this, &PlayerController::durationChanged);
    connect(core_, &PlayerCore::playbackRateChanged, this, &PlayerController::playbackRateChanged);

    coreThread_->start();
}

PlayerController::~PlayerController() {
    stop();
    if (coreThread_ != nullptr) {
        coreThread_->quit();
        coreThread_->wait();
    }
}

void PlayerController::open(const QString& localFilePath) {
    if (core_ == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(core_, "open", Qt::QueuedConnection, Q_ARG(QString, localFilePath));
}

void PlayerController::play() {
    if (core_ == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(core_, "play", Qt::QueuedConnection);
}

void PlayerController::pause() {
    if (core_ == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(core_, "pause", Qt::QueuedConnection);
}

void PlayerController::togglePlayPause() {
    if (playbackState_ == PlaybackState::Playing) {
        pause();
        return;
    }
    play();
}

void PlayerController::stop() {
    if (core_ == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(core_, "stop", Qt::BlockingQueuedConnection);
}

void PlayerController::seek(const double targetSec) {
    if (core_ == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(core_, "seek", Qt::QueuedConnection, Q_ARG(double, targetSec));
}

void PlayerController::setVolume(const float volume) {
    if (core_ == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(core_, "setVolume", Qt::QueuedConnection, Q_ARG(float, volume));
}

void PlayerController::setMuted(const bool muted) {
    if (core_ == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(core_, "setMuted", Qt::QueuedConnection, Q_ARG(bool, muted));
}

void PlayerController::setPlaybackRate(const double rate) {
    if (core_ == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(core_, "setPlaybackRate", Qt::QueuedConnection, Q_ARG(double, rate));
}

}  // namespace playerlab::core
