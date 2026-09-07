#include "core/PlayerCore.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include <algorithm>
#include <array>
#include <cmath>

#include "audio/QtAudioOutput.h"
#include "ffmpeg/FFmpegGlobal.h"
#include "utils/Logger.h"

namespace playerlab::core {

namespace {

constexpr std::array<double, 5> kSupportedPlaybackRates{0.5, 1.0, 1.25, 1.5, 2.0};

}  // namespace

PlayerCore::PlayerCore(QObject* parent) : QObject(parent) {
    playerlab::ffmpeg::FFmpegGlobal::initialize();

    audioOutput_ = std::make_unique<playerlab::audio::QtAudioOutput>();

    framePumpTimer_ = new QTimer(this);
    framePumpTimer_->setInterval(15);
    connect(framePumpTimer_, &QTimer::timeout, this, &PlayerCore::onFramePump);

    audioPumpTimer_ = new QTimer(this);
    audioPumpTimer_->setInterval(10);
    connect(audioPumpTimer_, &QTimer::timeout, this, &PlayerCore::onAudioPump);
}

PlayerCore::~PlayerCore() {
    stop();
}

void PlayerCore::open(const QString& localFilePath) {
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

void PlayerCore::play() {
    if (playbackState_ == PlaybackState::Playing) {
        return;
    }

    if (playbackState_ == PlaybackState::Paused) {
        playbackClock_.onPauseChanged(false);
        frameWakePending_ = false;
        pausedSeekPreviewPending_ = false;
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

void PlayerCore::pause() {
    if (playbackState_ != PlaybackState::Playing) {
        return;
    }

    playbackClock_.onPauseChanged(true);
    frameWakePending_ = false;
    if (audioOutput_ != nullptr) {
        audioOutput_->pause();
    }

    updateState(PlaybackState::Paused);
}

void PlayerCore::stop() {
    teardownPipeline();
    playbackClock_.reset(false);
    resetFrameTimeline();
    pendingVideoFrame_.reset();
    firstAudioPtsSec_.reset();
    pausedSeekPreviewPending_ = false;
    updateState(PlaybackState::Stopped);
    emit positionChanged(0.0, durationSec_);
}

void PlayerCore::seek(const double targetSec) {
    if (!hasSource_ || playbackState_ == PlaybackState::Stopped) {
        return;
    }

    const double clampedTarget = clampSeekTarget(targetSec);
    const bool keepPaused = playbackState_ == PlaybackState::Paused;
    const int nextSerial = readWorker_.requestSeek(playerlab::ffmpeg::SeekRequest{
        .targetSec = clampedTarget,
        .relSec = 0.0,
        .flags = AVSEEK_FLAG_BACKWARD,
    });
    resetSeekState(clampedTarget, nextSerial, keepPaused);
    emit positionChanged(clampedTarget, durationSec_);
}

void PlayerCore::setVolume(const float volume) {
    volume_ = std::clamp(volume, 0.0F, 1.0F);
    if (audioOutput_ != nullptr) {
        audioOutput_->setVolume(volume_);
    }
}

void PlayerCore::setMuted(const bool muted) {
    muted_ = muted;
    if (audioOutput_ != nullptr) {
        audioOutput_->setMuted(muted_);
    }
}

void PlayerCore::setPlaybackRate(const double rate) {
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

bool PlayerCore::startPipeline(const double startPositionSec, const bool refreshMediaInfo, const bool emitMediaInfo) {
    teardownPipeline();

    if (!hasSource_) {
        return false;
    }

    std::string error;
    playerlab::core::MediaInfo info = currentMediaInfo_;
    if (refreshMediaInfo || !hasMediaInfo_) {
        info = {};
    }

    if (!readWorker_.open(currentSource_, info, error, startPositionSec)) {
        emit openFailed(QString::fromStdString(error));
        updateState(PlaybackState::Stopped);
        emit positionChanged(0.0, durationSec_);
        return false;
    }

    currentMediaInfo_ = info;
    durationSec_ = std::max(0.0, static_cast<double>(currentMediaInfo_.durationMs) / 1000.0);
    hasMediaInfo_ = true;
    hasAudio_ = currentMediaInfo_.hasAudio && readWorker_.audioStreamIndex() >= 0;

    videoPacketQueue_.reset();
    audioPacketQueue_.reset();

    if (!videoDecoder_.open(readWorker_.videoCodecParameters(), readWorker_.videoTimeBase(), error)) {
        readWorker_.stop();
        emit openFailed(QString::fromStdString(error));
        updateState(PlaybackState::Stopped);
        emit positionChanged(0.0, durationSec_);
        return false;
    }

    if (hasAudio_) {
        if (!audioDecoder_.open(readWorker_.audioCodecParameters(), readWorker_.audioTimeBase(), error,
                                playbackRate_)) {
            videoDecoder_.stop();
            readWorker_.stop();
            emit openFailed(QString::fromStdString(error));
            updateState(PlaybackState::Stopped);
            emit positionChanged(0.0, durationSec_);
            return false;
        }

        if (audioOutput_ == nullptr ||
            !audioOutput_->open(48000, 2, playerlab::core::AudioSampleFormat::S16)) {
            audioDecoder_.stop();
            videoDecoder_.stop();
            readWorker_.stop();
            emit openFailed("open audio output failed");
            updateState(PlaybackState::Stopped);
            emit positionChanged(0.0, durationSec_);
            return false;
        }

        audioOutput_->setVolume(volume_);
        audioOutput_->setMuted(muted_);
    }

    readWorker_.start(&videoPacketQueue_, hasAudio_ ? &audioPacketQueue_ : nullptr);
    videoDecoder_.start(&videoPacketQueue_);
    if (hasAudio_) {
        audioDecoder_.start(&audioPacketQueue_);
    }

    activeSerial_ = readWorker_.currentSerial();
    pausedSeekPreviewPending_ = false;

    playbackClock_.reset(hasAudio_);
    playbackClock_.setPlaybackRate(playbackRate_);
    if (startPositionSec > 0.0) {
        playbackClock_.ensureSystemClockStarted(startPositionSec);
    }

    pendingVideoFrame_.reset();
    resetFrameTimeline();
    firstAudioPtsSec_.reset();
    lastSyncLogAt_ = std::chrono::steady_clock::now();
    debugVideoDisplayCount_ = 0;
    debugVideoDropCount_ = 0;
    debugVideoWaitCount_ = 0;
    debugAudioPumpCount_ = 0;
    debugVideoLateCount_ = 0;

    if (playerlab::utils::Logger::isPipelineDebugEnabled()) {
        LOG_DEBUG("Playback open: path='{}' hasVideo={} hasAudio={} durationSec={:.3f} seek={:.3f}", currentSource_.uri,
                  currentMediaInfo_.hasVideo, currentMediaInfo_.hasAudio, durationSec_, startPositionSec);
    }

    if (emitMediaInfo) {
        emit mediaInfoChanged(currentMediaInfo_);
    }
    emit durationChanged(durationSec_);
    emit positionChanged(startPositionSec, durationSec_);

    framePumpTimer_->start();
    if (hasAudio_) {
        audioPumpTimer_->start();
    }

    return true;
}

void PlayerCore::teardownPipeline() {
    if (framePumpTimer_ != nullptr) {
        framePumpTimer_->stop();
    }
    if (audioPumpTimer_ != nullptr) {
        audioPumpTimer_->stop();
    }

    videoPacketQueue_.abort();
    audioPacketQueue_.abort();
    readWorker_.stop();
    videoDecoder_.stop();
    audioDecoder_.stop();
    clearPacketQueues();

    if (audioOutput_ != nullptr) {
        audioOutput_->stop();
        audioOutput_->close();
    }

    pendingVideoFrame_.reset();
    frameWakePending_ = false;
    firstAudioPtsSec_.reset();
    pausedSeekPreviewPending_ = false;
    hasAudio_ = false;
    activeSerial_ = 1;

    if (playerlab::utils::Logger::isPipelineDebugEnabled()) {
        LOG_DEBUG("Playback pipeline teardown");
    }
}

void PlayerCore::updateState(const PlaybackState nextState) {
    if (playbackState_ == nextState) {
        return;
    }

    playbackState_ = nextState;
    emit playbackStateChanged(playbackState_);
}

void PlayerCore::publishPosition() {
    emit positionChanged(currentPositionSec(), durationSec_);
}

void PlayerCore::maybeTransitionToEnded() {
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

void PlayerCore::onFramePump() {
    syncActiveSerial();

    if (playbackState_ == PlaybackState::Paused) {
        if (pausedSeekPreviewPending_) {
            playerlab::core::VideoFrame previewFrame;
            if (tryPopCurrentVideoFrame(previewFrame)) {
                emit videoFrameReady(previewFrame);
                pausedSeekPreviewPending_ = false;
                publishPosition();
                if (framePumpTimer_ != nullptr) {
                    framePumpTimer_->stop();
                }
            }
        }
        return;
    }

    if (playbackState_ != PlaybackState::Playing) {
        return;
    }
    frameWakePending_ = false;

    if (!pendingVideoFrame_.has_value()) {
        playerlab::core::VideoFrame frame;
        if (!tryPopCurrentVideoFrame(frame)) {
            publishPosition();
            maybeTransitionToEnded();
            return;
        }
        pendingVideoFrame_ = std::move(frame);
    }

    if (!hasAudio_) {
        playbackClock_.ensureSystemClockStarted(pendingVideoFrame_->ptsSec);
    }

    if (frameTimerSec_ <= 0.0) {
        frameTimerSec_ = pendingVideoFrame_->ptsSec;
    }

    constexpr int kMaxDropsPerTick = 5;
    int dropCount = 0;
    while (pendingVideoFrame_.has_value()) {
        const double masterClockSec = playbackClock_.masterClockSec();
        const double framePtsSec = pendingVideoFrame_->ptsSec;
        const double baseDelaySec = std::clamp(framePtsSec - frameTimerSec_, 1.0 / 120.0, 0.1);
        lastFrameDurationSec_ = baseDelaySec;
        const double targetDelaySec = avSynchronizer_.computeTargetDelay(baseDelaySec, framePtsSec, masterClockSec);
        const double targetDisplayTimeSec = frameTimerSec_ + targetDelaySec;
        const double waitSec = targetDisplayTimeSec - masterClockSec;

        if (playerlab::utils::Logger::isPipelineDebugEnabled()) {
            maybeLogSyncStats(masterClockSec, framePtsSec);
        }

        if (waitSec > 0.001) {
            if (playerlab::utils::Logger::isPipelineDebugEnabled()) {
                ++debugVideoWaitCount_;
            }
            scheduleFrameWake(delayToWaitMs(waitSec));
            publishPosition();
            maybeTransitionToEnded();
            return;
        }

        if (waitSec < -std::max(lastFrameDurationSec_, 0.03) && dropCount < kMaxDropsPerTick) {
            if (playerlab::utils::Logger::isPipelineDebugEnabled()) {
                ++debugVideoDropCount_;
                ++debugVideoLateCount_;
            }
            frameTimerSec_ = targetDisplayTimeSec;
            pendingVideoFrame_.reset();
            playerlab::core::VideoFrame nextFrame;
            if (!tryPopCurrentVideoFrame(nextFrame)) {
                publishPosition();
                maybeTransitionToEnded();
                return;
            }
            pendingVideoFrame_ = std::move(nextFrame);
            ++dropCount;
            continue;
        }

        if (playerlab::utils::Logger::isPipelineDebugEnabled()) {
            ++debugVideoDisplayCount_;
        }
        frameTimerSec_ = targetDisplayTimeSec;
        emit videoFrameReady(*pendingVideoFrame_);
        pendingVideoFrame_.reset();
        publishPosition();
        maybeTransitionToEnded();
        return;
    }
}

void PlayerCore::onAudioPump() {
    if (playbackState_ != PlaybackState::Playing || audioOutput_ == nullptr) {
        return;
    }

    syncActiveSerial();
    audioOutput_->pump();

    playerlab::core::AudioFrame frame;
    int frameCount = 0;
    constexpr int kMaxFramesPerTick = 8;
    while (frameCount < kMaxFramesPerTick && tryPopCurrentAudioFrame(frame)) {
        if (!firstAudioPtsSec_.has_value()) {
            firstAudioPtsSec_ = frame.ptsSec;
        }
        audioOutput_->write(frame);
        ++frameCount;
    }

    if (firstAudioPtsSec_.has_value()) {
        const std::optional<double> playedSec = audioOutput_->playedSeconds();
        if (playedSec.has_value()) {
            const double latencySec = std::max(0.0, audioOutput_->outputLatencySeconds());
            const double effectivePlayed = std::max(0.0, *playedSec - latencySec);
            playbackClock_.updateAudioClock(*firstAudioPtsSec_ + effectivePlayed * playbackRate_);
            if (playerlab::utils::Logger::isPipelineDebugEnabled()) {
                ++debugAudioPumpCount_;
                if ((debugAudioPumpCount_ % 50) == 0) {
                    LOG_TRACE(
                        "Audio clock: firstPts={:.3f}s played={:.3f}s latency={:.3f}s effective={:.3f}s speed={:.2f} masterFromAudio={:.3f}s queuedFramesPerTick={}",
                        *firstAudioPtsSec_, *playedSec, latencySec, effectivePlayed, playbackRate_,
                        *firstAudioPtsSec_ + effectivePlayed * playbackRate_, frameCount);
                }
            }
        }
    }

    publishPosition();
    maybeTransitionToEnded();
}

double PlayerCore::currentPositionSec() const {
    const double rawSec = playbackClock_.masterClockSec();
    if (durationSec_ <= 0.0) {
        return std::max(0.0, rawSec);
    }
    return std::clamp(rawSec, 0.0, durationSec_);
}

bool PlayerCore::isPipelineDrained() const {
    const bool readDrained = readWorker_.isEofSerial(activeSerial_);
    const bool videoDrained =
        readDrained && videoDecoder_.isDrained(activeSerial_) && !pendingVideoFrame_.has_value();
    const bool audioDrained = !hasAudio_ || (readDrained && audioDecoder_.isDrained(activeSerial_));
    return videoDrained && audioDrained;
}

double PlayerCore::clampPlaybackRate(const double rate) {
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

double PlayerCore::clampSeekTarget(const double targetSec) const {
    if (durationSec_ <= 0.0) {
        return std::max(0.0, targetSec);
    }

    const double maxSeek = std::max(0.0, durationSec_ - 0.001);
    return std::clamp(targetSec, 0.0, maxSeek);
}

void PlayerCore::maybeLogSyncStats(const double masterClockSec, const double videoPtsSec) {
    const auto now = std::chrono::steady_clock::now();
    if (lastSyncLogAt_.time_since_epoch().count() == 0) {
        lastSyncLogAt_ = now;
        return;
    }
    const auto elapsed = now - lastSyncLogAt_;
    if (elapsed < std::chrono::seconds(1)) {
        return;
    }

    LOG_DEBUG(
        "AV sync stats(1s): display={} drop={} wait={} late={} master={:.3f}s video={:.3f}s diff={:.1f}ms frameTimer={:.3f}s lastDur={:.3f}s videoQ={} audioQ={} serial={}",
        debugVideoDisplayCount_, debugVideoDropCount_, debugVideoWaitCount_, debugVideoLateCount_, masterClockSec,
        videoPtsSec, (videoPtsSec - masterClockSec) * 1000.0, frameTimerSec_, lastFrameDurationSec_,
        videoPacketQueue_.size(), audioPacketQueue_.size(), activeSerial_);

    debugVideoDisplayCount_ = 0;
    debugVideoDropCount_ = 0;
    debugVideoWaitCount_ = 0;
    debugVideoLateCount_ = 0;
    lastSyncLogAt_ = now;
}

int PlayerCore::delayToWaitMs(const double delaySec) {
    const int waitMs = static_cast<int>(std::lround(delaySec * 1000.0));
    return std::clamp(waitMs, 1, 250);
}

void PlayerCore::scheduleFrameWake(const int waitMs) {
    if (frameWakePending_) {
        return;
    }
    frameWakePending_ = true;
    QTimer::singleShot(waitMs, this, [this]() {
        frameWakePending_ = false;
        onFramePump();
    });
}

void PlayerCore::resetFrameTimeline() {
    frameTimerSec_ = 0.0;
    lastFrameDurationSec_ = 1.0 / 30.0;
    frameWakePending_ = false;
}

void PlayerCore::resetSeekState(const double targetSec, const int nextSerial, const bool keepPaused) {
    activeSerial_ = nextSerial;
    pendingVideoFrame_.reset();
    resetFrameTimeline();
    firstAudioPtsSec_.reset();
    pausedSeekPreviewPending_ = keepPaused;

    playbackClock_.reset(hasAudio_);
    playbackClock_.setPlaybackRate(playbackRate_);
    playbackClock_.ensureSystemClockStarted(targetSec);

    if (audioOutput_ != nullptr) {
        audioOutput_->stop();
        if (keepPaused) {
            audioOutput_->pause();
        }
    }

    if (playerlab::utils::Logger::isPipelineDebugEnabled()) {
        LOG_DEBUG("PlayerCore begin seek reset: target={:.3f} serial={} paused={}", targetSec, nextSerial, keepPaused);
    }

    if (keepPaused) {
        playbackClock_.onPauseChanged(true);
        updateState(PlaybackState::Paused);
        if (framePumpTimer_ != nullptr) {
            framePumpTimer_->start();
        }
        return;
    }

    updateState(PlaybackState::Playing);
}

void PlayerCore::syncActiveSerial() {
    const int workerSerial = readWorker_.currentSerial();
    if (workerSerial <= activeSerial_) {
        return;
    }

    activeSerial_ = workerSerial;
    pendingVideoFrame_.reset();
    resetFrameTimeline();
    firstAudioPtsSec_.reset();
    if (playerlab::utils::Logger::isPipelineDebugEnabled()) {
        LOG_DEBUG("PlayerCore serial advanced: serial={}", activeSerial_);
    }
}

bool PlayerCore::tryPopCurrentVideoFrame(playerlab::core::VideoFrame& outFrame) {
    playerlab::core::VideoFrame frame;
    while (videoDecoder_.tryPopFrame(frame)) {
        if (frame.serial == activeSerial_) {
            outFrame = std::move(frame);
            return true;
        }
        if (playerlab::utils::Logger::isPipelineDebugEnabled()) {
            LOG_TRACE("Drop obsolete video frame: frameSerial={} activeSerial={} pts={:.3f}", frame.serial,
                      activeSerial_, frame.ptsSec);
        }
    }
    return false;
}

bool PlayerCore::tryPopCurrentAudioFrame(playerlab::core::AudioFrame& outFrame) {
    playerlab::core::AudioFrame frame;
    while (audioDecoder_.tryPopFrame(frame)) {
        if (frame.serial == activeSerial_) {
            outFrame = std::move(frame);
            return true;
        }
        if (playerlab::utils::Logger::isPipelineDebugEnabled()) {
            LOG_TRACE("Drop obsolete audio frame: frameSerial={} activeSerial={} pts={:.3f}", frame.serial,
                      activeSerial_, frame.ptsSec);
        }
    }
    return false;
}

void PlayerCore::clearPacketQueues() {
    videoPacketQueue_.clearWith([](playerlab::ffmpeg::QueuedPacket& queuedPacket) {
        if (queuedPacket.packet != nullptr) {
            av_packet_free(&queuedPacket.packet);
        }
    });
    audioPacketQueue_.clearWith([](playerlab::ffmpeg::QueuedPacket& queuedPacket) {
        if (queuedPacket.packet != nullptr) {
            av_packet_free(&queuedPacket.packet);
        }
    });
}

}  // namespace playerlab::core
