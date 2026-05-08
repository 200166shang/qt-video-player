#pragma once

#include <QObject>
#include <QTimer>
#include <QString>
#include <QMetaType>

#include "core/VideoFrame.h"
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

signals:
    void mediaInfoChanged(const playerlab::core::MediaInfo& info);
    void videoFrameReady(const playerlab::core::VideoFrame& frame);
    void openFailed(const QString& errorMessage);

private slots:
    void onFramePump();

private:
    playerlab::ffmpeg::FFmpegDemuxer demuxer_;
    playerlab::ffmpeg::FFmpegVideoDecoder videoDecoder_;
    QTimer framePumpTimer_;
};

}  // namespace playerlab::core

Q_DECLARE_METATYPE(playerlab::core::VideoFrame)
