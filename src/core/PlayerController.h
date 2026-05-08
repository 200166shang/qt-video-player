#pragma once

#include <QObject>
#include <QMetaType>
#include <QString>
#include <QTimer>

#include <memory>

#include "audio/IAudioOutput.h"
#include "core/AudioFrame.h"
#include "core/VideoFrame.h"
#include "ffmpeg/FFmpegAudioDecoder.h"
#include "ffmpeg/FFmpegDemuxer.h"
#include "ffmpeg/FFmpegVideoDecoder.h"

namespace playerlab::core {

class PlayerController : public QObject {
    Q_OBJECT

public:
    explicit PlayerController(QObject* parent = nullptr);
    ~PlayerController() override;

    void open(const QString& localFilePath);
    void stop();
    void setVolume(float volume);
    void setMuted(bool muted);
    void setPaused(bool paused);

signals:
    void mediaInfoChanged(const playerlab::core::MediaInfo& info);
    void videoFrameReady(const playerlab::core::VideoFrame& frame);
    void openFailed(const QString& errorMessage);

private slots:
    void onFramePump();
    void onAudioPump();

private:
    playerlab::ffmpeg::FFmpegDemuxer demuxer_;
    playerlab::ffmpeg::FFmpegVideoDecoder videoDecoder_;
    playerlab::ffmpeg::FFmpegAudioDecoder audioDecoder_;
    std::unique_ptr<playerlab::audio::IAudioOutput> audioOutput_;
    QTimer framePumpTimer_;
    QTimer audioPumpTimer_;
    float volume_ = 1.0F;
    bool muted_ = false;
    bool paused_ = false;
};

}  // namespace playerlab::core

Q_DECLARE_METATYPE(playerlab::core::VideoFrame)
