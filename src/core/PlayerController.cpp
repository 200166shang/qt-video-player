#include "core/PlayerController.h"

#include <algorithm>
#include <array>
#include <cmath>

#include <QMetaType>

#include "audio/QtAudioOutput.h"
#include "ffmpeg/FFmpegGlobal.h"
#include "utils/Logger.h"

namespace playerlab::core {

namespace {

constexpr std::array<double, 5> kSupportedPlaybackRates{0.5, 1.0, 1.25, 1.5, 2.0};

}  // namespace

PlayerController::PlayerController(QObject* parent) : QObject(parent) {
    playerlab::ffmpeg::FFmpegGlobal::initialize();
    qRegisterMetaType<playerlab::core::VideoFrame>("playerlab::core::VideoFrame");
    qRegisterMetaType<playerlab::core::PlayerController::PlaybackState>(
        "playerlab::core::PlayerController::PlaybackState");

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
    if (localFilePath.isEmpty()) {
        return;
    }

    currentSource_ = playerlab::core::MediaSource{
        .uri = localFilePath.toStdString(),
    };
    hasSource_ = true;
    hasMediaInfo_ = false;

    if (!startPipeline(0.0, true, true)) {
        return;
    }

    updateState(PlaybackState::Playing);
}

void PlayerController::play() {
    if (playbackState_ == PlaybackState::Playing) {
        return;
    }

    if (playbackState_ == PlaybackState::Paused) {
        playbackClock_.onPauseChanged(false);
        if (audioOutput_ != nullptr) {
            audioOutput_->resume();
        }
        updateState(PlaybackState::Playing);
        return;
    }

    if (!hasSource_) {
        return;
    }

    if (!startPipeline(0.0, false, false)) {
        return;
    }

    updateState(PlaybackState::Playing);
}

void PlayerController::pause() {
    if (playbackState_ != PlaybackState::Playing) {
        return;
    }

    playbackClock_.onPauseChanged(true);
    if (audioOutput_ != nullptr) {
        audioOutput_->pause();
    }

    updateState(PlaybackState::Paused);
}

void PlayerController::togglePlayPause() {
    if (playbackState_ == PlaybackState::Playing) {
        pause();
        return;
    }
    play();
}

void PlayerController::stop() {
    teardownPipeline();
    playbackClock_.reset(false);
    pendingVideoFrame_.reset();
    firstAudioPtsSec_.reset();
    updateState(PlaybackState::Stopped);
    emit positionChanged(0.0, durationSec_);
}

void PlayerController::seek(const double targetSec) {
    if (!hasSource_) {
        return;
    }

    const bool shouldResume = playbackState_ != PlaybackState::Paused;
    const double clampedTarget = clampSeekTarget(targetSec);
    if (!startPipeline(clampedTarget, false, false)) {
        return;
    }

    if (shouldResume) {
        updateState(PlaybackState::Playing);
        return;
    }

    playbackClock_.onPauseChanged(true);
    if (audioOutput_ != nullptr) {
        audioOutput_->pause();
    }
    updateState(PlaybackState::Paused);
}

void PlayerController::setVolume(const float volume) {
    volume_ = std::clamp(volume, 0.0F, 1.0F);
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

void PlayerController::setPlaybackRate(const double rate) {
    const double clampedRate = clampPlaybackRate(rate);
    if (std::abs(playbackRate_ - clampedRate) < 0.0001) {
        return;
    }

    const double oldRate = playbackRate_;
    const bool isActive = playbackState_ == PlaybackState::Playing || playbackState_ == PlaybackState::Paused;
    const bool needPipelineRestart = hasSource_ && hasAudio_ && isActive;
    const bool shouldResume = playbackState_ != PlaybackState::Paused;
    const double seekTarget = currentPositionSec();

    playbackRate_ = clampedRate;

    if (needPipelineRestart) {
        if (!startPipeline(seekTarget, false, false)) {
            playbackRate_ = oldRate;
            playbackClock_.setPlaybackRate(playbackRate_);
            emit playbackRateChanged(playbackRate_);
            return;
        }

        if (shouldResume) {
            updateState(PlaybackState::Playing);
        } else {
            playbackClock_.onPauseChanged(true);
            if (audioOutput_ != nullptr) {
                audioOutput_->pause();
            }
            updateState(PlaybackState::Paused);
        }
    } else {
        playbackClock_.setPlaybackRate(playbackRate_);
    }

    emit playbackRateChanged(playbackRate_);
    publishPosition();
}

bool PlayerController::startPipeline(const double startPositionSec, const bool refreshMediaInfo, const bool emitMediaInfo) {
    teardownPipeline();

    if (!hasSource_) {
        return false;
    }

    std::string error;
    if (refreshMediaInfo || !hasMediaInfo_) {
        playerlab::core::MediaInfo info;
        if (!demuxer_.open(currentSource_, info, error)) {
            emit openFailed(QString::fromStdString(error));
            updateState(PlaybackState::Stopped);
            emit positionChanged(0.0, durationSec_);
            return false;
        }
        currentMediaInfo_ = info;
        durationSec_ = std::max(0.0, static_cast<double>(currentMediaInfo_.durationMs) / 1000.0);
        hasMediaInfo_ = true;
    }
    hasAudio_ = currentMediaInfo_.hasAudio;

    if (!videoDecoder_.open(currentSource_, error, startPositionSec)) {
        emit openFailed(QString::fromStdString(error));
        updateState(PlaybackState::Stopped);
        emit positionChanged(0.0, durationSec_);
        return false;
    }

    if (hasAudio_) {
        if (!audioDecoder_.open(currentSource_, error, startPositionSec, playbackRate_)) {
            videoDecoder_.stop();
            emit openFailed(QString::fromStdString(error));
            updateState(PlaybackState::Stopped);
            emit positionChanged(0.0, durationSec_);
            return false;
        }

        if (audioOutput_ == nullptr ||
            !audioOutput_->open(48000, 2, playerlab::core::AudioSampleFormat::S16)) {
            audioDecoder_.stop();
            videoDecoder_.stop();
            emit openFailed("open audio output failed");
            updateState(PlaybackState::Stopped);
            emit positionChanged(0.0, durationSec_);
            return false;
        }

        audioOutput_->setVolume(volume_);
        audioOutput_->setMuted(muted_);
    }

    playbackClock_.reset(hasAudio_);
    playbackClock_.setPlaybackRate(playbackRate_);
    if (startPositionSec > 0.0) {
        playbackClock_.ensureSystemClockStarted(startPositionSec);
    }

    pendingVideoFrame_.reset();
    firstAudioPtsSec_.reset();
    lastSyncLogAt_ = std::chrono::steady_clock::now();
    debugVideoDisplayCount_ = 0;
    debugVideoDropCount_ = 0;
    debugVideoWaitCount_ = 0;
    debugAudioPumpCount_ = 0;

    if (playerlab::utils::Logger::isPipelineDebugEnabled()) {
        LOG_DEBUG("Playback open: path='{}' hasVideo={} hasAudio={} durationSec={:.3f} seek={:.3f}", currentSource_.uri,
                  currentMediaInfo_.hasVideo, currentMediaInfo_.hasAudio, durationSec_, startPositionSec);
    }

    if (emitMediaInfo) {
        emit mediaInfoChanged(currentMediaInfo_);
    }
    emit durationChanged(durationSec_);
    emit positionChanged(startPositionSec, durationSec_);

    framePumpTimer_.start();
    if (hasAudio_) {
        audioPumpTimer_.start();
    }

    return true;
}

void PlayerController::teardownPipeline() {
    framePumpTimer_.stop();
    audioPumpTimer_.stop();
    videoDecoder_.stop();
    audioDecoder_.stop();
    if (audioOutput_ != nullptr) {
        audioOutput_->stop();
        audioOutput_->close();
    }

    pendingVideoFrame_.reset();
    firstAudioPtsSec_.reset();
    hasAudio_ = false;

    if (playerlab::utils::Logger::isPipelineDebugEnabled()) {
        LOG_DEBUG("Playback pipeline teardown");
    }
}

void PlayerController::updateState(const PlaybackState nextState) {
    if (playbackState_ == nextState) {
        return;
    }

    playbackState_ = nextState;
    emit playbackStateChanged(playbackState_);
}

void PlayerController::publishPosition() {
    emit positionChanged(currentPositionSec(), durationSec_);
}

void PlayerController::maybeTransitionToEnded() {
    if (playbackState_ != PlaybackState::Playing) {
        return;
    }

    if (!isPipelineDrained()) {
        return;
    }

    const double currentSec = currentPositionSec();
    if (durationSec_ > 0.0 && currentSec + 0.05 < durationSec_) {
        return;
    }

    teardownPipeline();
    playbackClock_.reset(false);
    updateState(PlaybackState::Ended);
    emit positionChanged(durationSec_ > 0.0 ? durationSec_ : currentSec, durationSec_);
}

void PlayerController::onFramePump() {
    if (playbackState_ != PlaybackState::Playing) {
        return;
    }

    if (!pendingVideoFrame_.has_value()) {
        playerlab::core::VideoFrame frame;
        if (!videoDecoder_.tryPopFrame(frame)) {
            publishPosition();
            maybeTransitionToEnded();
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
            publishPosition();
            maybeTransitionToEnded();
            return;
        }

        if (decision.action == playerlab::core::AVSynchronizer::VideoAction::Display) {
            if (playerlab::utils::Logger::isPipelineDebugEnabled()) {
                ++debugVideoDisplayCount_;
            }
            emit videoFrameReady(*pendingVideoFrame_);
            pendingVideoFrame_.reset();
            publishPosition();
            maybeTransitionToEnded();
            return;
        }

        if (playerlab::utils::Logger::isPipelineDebugEnabled()) {
            ++debugVideoDropCount_;
        }
        pendingVideoFrame_.reset();
        playerlab::core::VideoFrame nextFrame;
        if (!videoDecoder_.tryPopFrame(nextFrame)) {
            publishPosition();
            maybeTransitionToEnded();
            return;
        }
        pendingVideoFrame_ = std::move(nextFrame);
        ++dropCount;
        if (dropCount >= kMaxDropsPerTick) {
            publishPosition();
            maybeTransitionToEnded();
            return;
        }
    }
}

void PlayerController::onAudioPump() {
    if (playbackState_ != PlaybackState::Playing || audioOutput_ == nullptr) {
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
            playbackClock_.updateAudioClock(*firstAudioPtsSec_ + *playedSec * playbackRate_);
            if (playerlab::utils::Logger::isPipelineDebugEnabled()) {
                ++debugAudioPumpCount_;
                if ((debugAudioPumpCount_ % 50) == 0) {
                    LOG_TRACE(
                        "Audio clock: firstPts={:.3f}s played={:.3f}s speed={:.2f} masterFromAudio={:.3f}s queuedFramesPerTick={}"
                        ,
                        *firstAudioPtsSec_, *playedSec, playbackRate_, *firstAudioPtsSec_ + *playedSec * playbackRate_,
                        frameCount);
                }
            }
        }
    }

    publishPosition();
    maybeTransitionToEnded();
}

double PlayerController::currentPositionSec() const {
    const double rawSec = playbackClock_.masterClockSec();
    if (durationSec_ <= 0.0) {
        return std::max(0.0, rawSec);
    }
    return std::clamp(rawSec, 0.0, durationSec_);
}

bool PlayerController::isPipelineDrained() const {
    const bool videoDrained = videoDecoder_.isDrained() && !pendingVideoFrame_.has_value();
    const bool audioDrained = !hasAudio_ || audioDecoder_.isDrained();
    return videoDrained && audioDrained;
}

double PlayerController::clampPlaybackRate(const double rate) {
    auto closest = kSupportedPlaybackRates.front();
    auto closestDiff = std::abs(rate - closest);
    for (const double candidate : kSupportedPlaybackRates) {
        const auto diff = std::abs(rate - candidate);
        if (diff < closestDiff) {
            closest = candidate;
            closestDiff = diff;
        }
    }
    return closest;
}

double PlayerController::clampSeekTarget(const double targetSec) const {
    if (durationSec_ <= 0.0) {
        return std::max(0.0, targetSec);
    }

    // Avoid seeking exactly to duration to keep decoder loops in a valid range.
    const double maxSeek = std::max(0.0, durationSec_ - 0.001);
    return std::clamp(targetSec, 0.0, maxSeek);
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
    LOG_DEBUG("AV sync stats(1s): display={} drop={} wait={} master={:.3f}s video={:.3f}s diff={:.1f}ms",
              debugVideoDisplayCount_, debugVideoDropCount_, debugVideoWaitCount_, masterClockSec, videoPtsSec, diffMs);

    debugVideoDisplayCount_ = 0;
    debugVideoDropCount_ = 0;
    debugVideoWaitCount_ = 0;
    lastSyncLogAt_ = now;
}

}  // namespace playerlab::core
