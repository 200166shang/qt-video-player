#include "core/PlayerController.h"

#include "ffmpeg/FFmpegGlobal.h"

namespace playerlab::core {

PlayerController::PlayerController(QObject* parent) : QObject(parent) {
    playerlab::ffmpeg::FFmpegGlobal::initialize();
}

void PlayerController::open(const QString& localFilePath) {
    playerlab::core::MediaSource source{
        .uri = localFilePath.toStdString(),
    };

    playerlab::core::MediaInfo info;
    std::string error;
    if (!demuxer_.open(source, info, error)) {
        emit openFailed(QString::fromStdString(error));
        return;
    }

    emit mediaInfoChanged(info);
}

}  // namespace playerlab::core
