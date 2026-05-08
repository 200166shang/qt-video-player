#include "core/PlayerController.h"

#include <QMetaType>

#include "ffmpeg/FFmpegGlobal.h"

namespace playerlab::core {

PlayerController::PlayerController(QObject* parent) : QObject(parent) {
    playerlab::ffmpeg::FFmpegGlobal::initialize();
    qRegisterMetaType<playerlab::core::VideoFrame>("playerlab::core::VideoFrame");

    framePumpTimer_.setInterval(15);
    connect(&framePumpTimer_, &QTimer::timeout, this, &PlayerController::onFramePump);
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

    emit mediaInfoChanged(info);
    framePumpTimer_.start();
}

void PlayerController::stop() {
    framePumpTimer_.stop();
    videoDecoder_.stop();
}

void PlayerController::onFramePump() {
    playerlab::core::VideoFrame frame;
    bool gotFrame = false;
    while (videoDecoder_.tryPopFrame(frame)) {
        gotFrame = true;
    }

    if (gotFrame) {
        emit videoFrameReady(frame);
    }
}

}  // namespace playerlab::core
