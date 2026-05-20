#pragma once

#include <QObject>
#include <QMetaType>
#include <QString>
#include <QThread>

#include "core/MediaInfo.h"
#include "core/MediaSource.h"
#include "core/PlaybackState.h"
#include "core/VideoFrame.h"

namespace playerlab::core {

class PlayerCore;

class PlayerController : public QObject {
    Q_OBJECT

public:
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
    void playbackStateChanged(playerlab::core::PlaybackState state);
    void positionChanged(double currentSec, double durationSec);
    void durationChanged(double durationSec);
    void playbackRateChanged(double rate);

private:
    QThread* coreThread_ = nullptr;
    PlayerCore* core_ = nullptr;
    PlaybackState playbackState_ = PlaybackState::Stopped;
};

}  // namespace playerlab::core

Q_DECLARE_METATYPE(playerlab::core::VideoFrame)
Q_DECLARE_METATYPE(playerlab::core::PlaybackState)
