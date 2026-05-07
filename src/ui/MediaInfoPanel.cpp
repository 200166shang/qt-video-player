#include "MediaInfoPanel.h"

#include <QLabel>
#include <QVBoxLayout>

#include <cmath>

#include "core/MediaInfo.h"

MediaInfoPanel::MediaInfoPanel(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->addWidget(new QLabel("Media Info", this));

    fileValue_ = new QLabel("File: --", this);
    formatValue_ = new QLabel("Format: --", this);
    durationValue_ = new QLabel("Duration: --", this);
    videoValue_ = new QLabel("Video: --", this);
    audioValue_ = new QLabel("Audio: --", this);
    errorValue_ = new QLabel("", this);
    errorValue_->setStyleSheet("color: #ff6b6b;");
    errorValue_->setWordWrap(true);

    layout->addWidget(fileValue_);
    layout->addWidget(formatValue_);
    layout->addWidget(durationValue_);
    layout->addWidget(videoValue_);
    layout->addWidget(audioValue_);
    layout->addWidget(errorValue_);
    layout->addStretch();

    clearInfo();
}

void MediaInfoPanel::clearInfo() {
    fileValue_->setText("File: --");
    formatValue_->setText("Format: --");
    durationValue_->setText("Duration: --");
    videoValue_->setText("Video: --");
    audioValue_->setText("Audio: --");
    errorValue_->clear();
}

void MediaInfoPanel::setMediaInfo(const playerlab::core::MediaInfo& info) {
    fileValue_->setText(QString("File: %1").arg(QString::fromStdString(info.filePath)));
    formatValue_->setText(QString("Format: %1").arg(QString::fromStdString(info.containerFormat)));

    const double durationSec = static_cast<double>(info.durationMs) / 1000.0;
    durationValue_->setText(QString("Duration: %1 s").arg(durationSec, 0, 'f', 2));

    if (info.hasVideo) {
        videoValue_->setText(QString("Video: %1, %2x%3, %4 fps")
                                 .arg(QString::fromStdString(info.videoCodec))
                                 .arg(info.videoWidth)
                                 .arg(info.videoHeight)
                                 .arg(info.frameRate, 0, 'f', 2));
    } else {
        videoValue_->setText("Video: --");
    }

    if (info.hasAudio) {
        audioValue_->setText(QString("Audio: %1, %2 Hz, %3 ch")
                                 .arg(QString::fromStdString(info.audioCodec))
                                 .arg(info.audioSampleRate)
                                 .arg(info.audioChannels));
    } else {
        audioValue_->setText("Audio: --");
    }

    errorValue_->clear();
}

void MediaInfoPanel::setError(const QString& message) {
    clearInfo();
    errorValue_->setText(QString("Error: %1").arg(message));
}
