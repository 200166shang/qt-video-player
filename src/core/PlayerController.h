#pragma once

#include <QObject>
#include <QString>

#include "ffmpeg/FFmpegDemuxer.h"

namespace playerlab::core {

class PlayerController : public QObject {
    Q_OBJECT

public:
    explicit PlayerController(QObject* parent = nullptr);

    void open(const QString& localFilePath);

signals:
    void mediaInfoChanged(const playerlab::core::MediaInfo& info);
    void openFailed(const QString& errorMessage);

private:
    playerlab::ffmpeg::FFmpegDemuxer demuxer_;
};

}  // namespace playerlab::core
