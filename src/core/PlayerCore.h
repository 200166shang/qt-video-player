#pragma once

#include <QObject>
#include <QString>
#include <QTimer>

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>

#include "audio/IAudioOutput.h"
#include "core/AVSynchronizer.h"
#include "core/MediaInfo.h"
#include "core/MediaSource.h"
#include "core/PlaybackClock.h"
#include "core/PlaybackState.h"
#include "core/VideoFrame.h"
#include "ffmpeg/FFmpegAudioDecoder.h"
#include "ffmpeg/FFmpegReadWorker.h"
#include "ffmpeg/FFmpegVideoDecoder.h"
#include "ffmpeg/PacketQueue.h"
#include "ffmpeg/QueuedPacket.h"

namespace playerlab::core {

class PlayerCore : public QObject {
    Q_OBJECT

public:
    explicit PlayerCore(QObject* parent = nullptr);
    ~PlayerCore() override;

public slots:
    void open(const QString& localFilePath);
    void play();
    void pause();
    void stop();
    void seek(double targetSec);
    void setVolume(float volume);
    void setMuted(bool muted);
    void setPlaybackRate(double rate);

signals:
    void mediaInfoChanged(const playerlab::core::MediaInfo& info);
    void videoFrameReady(const playerlab::core::VideoFrame& frame);
    void openFailed(const QString& errorMessage);
    void playbackStateChanged(playerlab::core::PlaybackState state);
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
    void resetSeekState(double targetSec, int nextSerial, bool keepPaused);
    void syncActiveSerial();
    [[nodiscard]] bool tryPopCurrentVideoFrame(playerlab::core::VideoFrame& outFrame);
    [[nodiscard]] bool tryPopCurrentAudioFrame(playerlab::core::AudioFrame& outFrame);
    void clearPacketQueues();

    std::unique_ptr<playerlab::audio::IAudioOutput> audioOutput_;
    QTimer* framePumpTimer_ = nullptr;
    QTimer* audioPumpTimer_ = nullptr;
    std::optional<playerlab::core::VideoFrame> pendingVideoFrame_;
    playerlab::core::PlaybackClock playbackClock_;
    playerlab::core::AVSynchronizer avSynchronizer_;
    playerlab::ffmpeg::FFmpegReadWorker readWorker_;
    playerlab::ffmpeg::FFmpegVideoDecoder videoDecoder_;
    playerlab::ffmpeg::FFmpegAudioDecoder audioDecoder_;
    playerlab::ffmpeg::PacketQueue<playerlab::ffmpeg::QueuedPacket> videoPacketQueue_{96};
    playerlab::ffmpeg::PacketQueue<playerlab::ffmpeg::QueuedPacket> audioPacketQueue_{192};
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
    playerlab::core::PlaybackState playbackState_ = playerlab::core::PlaybackState::Stopped;

    float volume_ = 1.0F;
    bool muted_ = false;
    std::optional<double> firstAudioPtsSec_;
    int activeSerial_ = 1;
    bool pausedSeekPreviewPending_ = false;
    std::chrono::steady_clock::time_point lastSyncLogAt_{};
    std::uint64_t debugVideoDisplayCount_ = 0;
    std::uint64_t debugVideoDropCount_ = 0;
    std::uint64_t debugVideoWaitCount_ = 0;
    std::uint64_t debugAudioPumpCount_ = 0;
    std::uint64_t debugVideoLateCount_ = 0;
};

}  // namespace playerlab::core
