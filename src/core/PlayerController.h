#pragma once

#include <QObject>
#include <QMetaType>
#include <QString>
#include <QTimer>

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>

#include "audio/IAudioOutput.h"
#include "core/AVSynchronizer.h"
#include "core/AudioFrame.h"
#include "core/MediaInfo.h"
#include "core/MediaSource.h"
#include "core/PlaybackClock.h"
#include "core/VideoFrame.h"
#include "ffmpeg/FFmpegAudioDecoder.h"
#include "ffmpeg/FFmpegDemuxer.h"
#include "ffmpeg/FFmpegVideoDecoder.h"

namespace playerlab::core {

class PlayerController : public QObject {
    Q_OBJECT

public:
    enum class PlaybackState {
        Stopped,
        Playing,
        Paused,
        Ended,
    };

    explicit PlayerController(QObject* parent = nullptr);
    ~PlayerController() override;

    void open(const QString& localFilePath);
    void play();
    void pause();
    void togglePlayPause();
    void stop();
    void seek(double targetSec);
    void setVolume(float volume);
    void setMuted(bool muted);
    void setPlaybackRate(double rate);

    [[nodiscard]] PlaybackState playbackState() const { return playbackState_; }

signals:
    void mediaInfoChanged(const playerlab::core::MediaInfo& info);
    void videoFrameReady(const playerlab::core::VideoFrame& frame);
    void openFailed(const QString& errorMessage);
    void playbackStateChanged(playerlab::core::PlayerController::PlaybackState state);
    void positionChanged(double currentSec, double durationSec);
    void durationChanged(double durationSec);
    void playbackRateChanged(double rate);

private slots:
    void onFramePump();
    void onAudioPump();

private:
    bool startPipeline(double startPositionSec, bool refreshMediaInfo, bool emitMediaInfo);
    void teardownPipeline();
    void updateState(PlaybackState nextState);
    void publishPosition();
    void maybeTransitionToEnded();
    [[nodiscard]] double currentPositionSec() const;
    [[nodiscard]] bool isPipelineDrained() const;
    [[nodiscard]] static double clampPlaybackRate(double rate);
    [[nodiscard]] double clampSeekTarget(double targetSec) const;
    void maybeLogSyncStats(double masterClockSec, double videoPtsSec);
    [[nodiscard]] static int delayToWaitMs(double delaySec);
    void scheduleFrameWake(int waitMs);
    void resetFrameTimeline();

    playerlab::ffmpeg::FFmpegDemuxer demuxer_;
    playerlab::ffmpeg::FFmpegVideoDecoder videoDecoder_;
    playerlab::ffmpeg::FFmpegAudioDecoder audioDecoder_;
    std::unique_ptr<playerlab::audio::IAudioOutput> audioOutput_;
    QTimer framePumpTimer_;
    QTimer audioPumpTimer_;
    std::optional<playerlab::core::VideoFrame> pendingVideoFrame_;
    playerlab::core::PlaybackClock playbackClock_;
    playerlab::core::AVSynchronizer avSynchronizer_;
    double frameTimerSec_ = 0.0;
    double lastFrameDurationSec_ = 1.0 / 30.0;
    bool frameWakePending_ = false;

    playerlab::core::MediaSource currentSource_;
    playerlab::core::MediaInfo currentMediaInfo_;
    bool hasSource_ = false;
    bool hasMediaInfo_ = false;
    bool hasAudio_ = false;
    double durationSec_ = 0.0;
    double playbackRate_ = 1.0;
    playerlab::core::PlayerController::PlaybackState playbackState_ =
        playerlab::core::PlayerController::PlaybackState::Stopped;

    float volume_ = 1.0F;
    bool muted_ = false;
    std::optional<double> firstAudioPtsSec_;
    std::chrono::steady_clock::time_point lastSyncLogAt_{};
    std::uint64_t debugVideoDisplayCount_ = 0;
    std::uint64_t debugVideoDropCount_ = 0;
    std::uint64_t debugVideoWaitCount_ = 0;
    std::uint64_t debugAudioPumpCount_ = 0;
    std::uint64_t debugVideoLateCount_ = 0;
};

}  // namespace playerlab::core

Q_DECLARE_METATYPE(playerlab::core::VideoFrame)
Q_DECLARE_METATYPE(playerlab::core::PlayerController::PlaybackState)
