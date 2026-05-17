#include "core/PlayerController.h"

#include <QMetaType>

#include "audio/QtAudioOutput.h"
#include "ffmpeg/FFmpegGlobal.h"
#include "utils/Logger.h"

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

    hasAudio_ = info.hasAudio;
    playbackClock_.reset(hasAudio_);
    pendingVideoFrame_.reset();
    firstAudioPtsSec_.reset();
    lastSyncLogAt_ = std::chrono::steady_clock::now();
    debugVideoDisplayCount_ = 0;
    debugVideoDropCount_ = 0;
    debugVideoWaitCount_ = 0;
    debugAudioPumpCount_ = 0;
    if (playerlab::utils::Logger::isPipelineDebugEnabled()) {
        LOG_DEBUG("Playback open: path='{}' hasVideo={} hasAudio={} durationSec={:.3f}", source.uri, info.hasVideo,
                  info.hasAudio, info.durationSec);
    }

    if (hasAudio_) {
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
    if (hasAudio_) {
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
    pendingVideoFrame_.reset();
    hasAudio_ = false;
    firstAudioPtsSec_.reset();
    playbackClock_.reset(false);
    if (playerlab::utils::Logger::isPipelineDebugEnabled()) {
        LOG_DEBUG("Playback stop");
    }
}

void PlayerController::onFramePump() {
    if (paused_) {
        return;
    }

    if (!pendingVideoFrame_.has_value()) {
        playerlab::core::VideoFrame frame;
        if (!videoDecoder_.tryPopFrame(frame)) {
            return;
        }
        pendingVideoFrame_ = std::move(frame);
    }

    if (!hasAudio_) {
        playbackClock_.ensureSystemClockStarted(pendingVideoFrame_->ptsSec);
    }

    constexpr int kMaxDropsPerTick = 5;
    int dropCount = 0;
    while (pendingVideoFrame_.has_value()) {
        const double masterClockSec = playbackClock_.masterClockSec();
        const playerlab::core::AVSynchronizer::VideoDecision decision =
            avSynchronizer_.decideVideoFrame(pendingVideoFrame_->ptsSec, masterClockSec);
        if (playerlab::utils::Logger::isPipelineDebugEnabled()) {
            maybeLogSyncStats(masterClockSec, pendingVideoFrame_->ptsSec);
        }

        if (decision.action == playerlab::core::AVSynchronizer::VideoAction::Wait) {
            if (playerlab::utils::Logger::isPipelineDebugEnabled()) {
                ++debugVideoWaitCount_;
            }
            return;
        }

        if (decision.action == playerlab::core::AVSynchronizer::VideoAction::Display) {
            if (playerlab::utils::Logger::isPipelineDebugEnabled()) {
                ++debugVideoDisplayCount_;
            }
            emit videoFrameReady(*pendingVideoFrame_);
            pendingVideoFrame_.reset();
            return;
        }

        if (playerlab::utils::Logger::isPipelineDebugEnabled()) {
            ++debugVideoDropCount_;
        }
        pendingVideoFrame_.reset();
        playerlab::core::VideoFrame nextFrame;
        if (!videoDecoder_.tryPopFrame(nextFrame)) {
            return;
        }
        pendingVideoFrame_ = std::move(nextFrame);
        ++dropCount;
        if (dropCount >= kMaxDropsPerTick) {
            return;
        }
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
        if (!firstAudioPtsSec_.has_value()) {
            firstAudioPtsSec_ = frame.ptsSec;
        }
        audioOutput_->write(frame);
        ++frameCount;
    }

    if (firstAudioPtsSec_.has_value()) {
        const std::optional<double> playedSec = audioOutput_->playedSeconds();
        if (playedSec.has_value()) {
            playbackClock_.updateAudioClock(*firstAudioPtsSec_ + *playedSec);
            if (playerlab::utils::Logger::isPipelineDebugEnabled()) {
                ++debugAudioPumpCount_;
                if ((debugAudioPumpCount_ % 50) == 0) {
                    LOG_TRACE(
                        "Audio clock: firstPts={:.3f}s played={:.3f}s masterFromAudio={:.3f}s queuedFramesPerTick={}",
                        *firstAudioPtsSec_, *playedSec, *firstAudioPtsSec_ + *playedSec, frameCount);
                }
            }
        }
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
    playbackClock_.onPauseChanged(paused);
    paused_ = paused;
    if (playerlab::utils::Logger::isPipelineDebugEnabled()) {
        LOG_DEBUG("Playback pause changed: paused={}", paused_);
    }
    if (audioOutput_ == nullptr) {
        return;
    }
    if (paused_) {
        audioOutput_->pause();
    } else {
        audioOutput_->resume();
    }
}

void PlayerController::maybeLogSyncStats(const double masterClockSec, const double videoPtsSec) {
    const auto now = std::chrono::steady_clock::now();
    if (lastSyncLogAt_.time_since_epoch().count() == 0) {
        lastSyncLogAt_ = now;
        return;
    }
    const auto elapsed = now - lastSyncLogAt_;
    if (elapsed < std::chrono::seconds(1)) {
        return;
    }

    const double diffMs = (videoPtsSec - masterClockSec) * 1000.0;
    LOG_DEBUG(
        "AV sync stats(1s): display={} drop={} wait={} master={:.3f}s video={:.3f}s diff={:.1f}ms",
        debugVideoDisplayCount_, debugVideoDropCount_, debugVideoWaitCount_, masterClockSec, videoPtsSec, diffMs);

    debugVideoDisplayCount_ = 0;
    debugVideoDropCount_ = 0;
    debugVideoWaitCount_ = 0;
    lastSyncLogAt_ = now;
}

}  // namespace playerlab::core
