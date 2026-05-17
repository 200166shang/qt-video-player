#include "ControlBar.h"

#include <algorithm>
#include <cmath>

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSlider>

ControlBar::ControlBar(QWidget* parent) : QWidget(parent) {
    openButton_ = new QPushButton("Open", this);
    playPauseButton_ = new QPushButton("Play", this);
    stopButton_ = new QPushButton("Stop", this);
    muteButton_ = new QPushButton("Mute", this);

    progressSlider_ = new QSlider(Qt::Horizontal, this);
    progressSlider_->setRange(0, 1000);
    progressSlider_->setValue(0);

    timeLabel_ = new QLabel("00:00 / 00:00", this);
    speedCombo_ = new QComboBox(this);
    speedCombo_->addItem("0.5x", 0.5);
    speedCombo_->addItem("1.0x", 1.0);
    speedCombo_->addItem("1.25x", 1.25);
    speedCombo_->addItem("1.5x", 1.5);
    speedCombo_->addItem("2.0x", 2.0);
    speedCombo_->setCurrentIndex(1);
    speedCombo_->setFixedWidth(84);

    volumeSlider_ = new QSlider(Qt::Horizontal, this);
    volumeSlider_->setRange(0, 100);
    volumeSlider_->setValue(100);
    volumeSlider_->setFixedWidth(140);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 8, 12, 8);
    layout->setSpacing(8);
    layout->addWidget(openButton_);
    layout->addWidget(playPauseButton_);
    layout->addWidget(stopButton_);
    layout->addWidget(progressSlider_, 1);
    layout->addWidget(timeLabel_);
    layout->addWidget(speedCombo_);
    layout->addWidget(muteButton_);
    layout->addWidget(volumeSlider_);

    connect(openButton_, &QPushButton::clicked, this, &ControlBar::onOpenClicked);
    connect(playPauseButton_, &QPushButton::clicked, this, &ControlBar::onPlayPauseClicked);
    connect(stopButton_, &QPushButton::clicked, this, &ControlBar::onStopClicked);
    connect(progressSlider_, &QSlider::sliderPressed, this, &ControlBar::onProgressPressed);
    connect(progressSlider_, &QSlider::sliderReleased, this, &ControlBar::onProgressReleased);
    connect(progressSlider_, &QSlider::valueChanged, this, &ControlBar::onProgressValueChanged);
    connect(speedCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this, &ControlBar::onPlaybackRateIndexChanged);
    connect(volumeSlider_, &QSlider::valueChanged, this, &ControlBar::onVolumeSliderChanged);
    connect(muteButton_, &QPushButton::clicked, this, &ControlBar::onMuteClicked);

    syncPlayPauseButtonText();
}

void ControlBar::setPlaybackState(const playerlab::core::PlayerController::PlaybackState state) {
    playbackState_ = state;
    syncPlayPauseButtonText();

    if (playbackState_ == playerlab::core::PlayerController::PlaybackState::Stopped) {
        setProgress(0.0, durationSec_);
    }
    if (playbackState_ == playerlab::core::PlayerController::PlaybackState::Ended) {
        setProgress(durationSec_, durationSec_);
    }
}

void ControlBar::setProgress(const double currentSec, const double durationSec) {
    currentSec_ = std::max(0.0, currentSec);
    durationSec_ = std::max(0.0, durationSec);

    if (!seeking_) {
        const QSignalBlocker blocker(progressSlider_);
        int sliderValue = 0;
        if (durationSec_ > 0.0) {
            const double ratio = std::clamp(currentSec_ / durationSec_, 0.0, 1.0);
            sliderValue = static_cast<int>(std::lround(ratio * 1000.0));
        }
        progressSlider_->setValue(sliderValue);
    }

    updateTimeLabel(currentSec_);
}

void ControlBar::setDuration(const double durationSec) {
    durationSec_ = std::max(0.0, durationSec);
    updateTimeLabel(currentSec_);
}

void ControlBar::setPlaybackRate(const double rate) {
    for (int i = 0; i < speedCombo_->count(); ++i) {
        const double value = speedCombo_->itemData(i).toDouble();
        if (std::abs(value - rate) < 0.0001) {
            const QSignalBlocker blocker(speedCombo_);
            speedCombo_->setCurrentIndex(i);
            return;
        }
    }
}

void ControlBar::onOpenClicked() {
    emit openRequested();
}

void ControlBar::onPlayPauseClicked() {
    emit playPauseRequested();
}

void ControlBar::onStopClicked() {
    emit stopRequested();
}

void ControlBar::onProgressPressed() {
    seeking_ = true;
}

void ControlBar::onProgressReleased() {
    if (durationSec_ <= 0.0) {
        seeking_ = false;
        return;
    }

    const double ratio = static_cast<double>(progressSlider_->value()) / 1000.0;
    const double targetSec = std::clamp(ratio * durationSec_, 0.0, durationSec_);
    seeking_ = false;
    emit seekRequested(targetSec);
}

void ControlBar::onProgressValueChanged(const int value) {
    if (!seeking_) {
        return;
    }

    const double ratio = static_cast<double>(value) / 1000.0;
    const double previewSec = std::clamp(ratio * durationSec_, 0.0, durationSec_);
    updateTimeLabel(previewSec);
}

void ControlBar::onVolumeSliderChanged(const int value) {
    emit volumeChanged(static_cast<float>(value) / 100.0F);
}

void ControlBar::onMuteClicked() {
    muted_ = !muted_;
    muteButton_->setText(muted_ ? "Unmute" : "Mute");
    emit mutedChanged(muted_);
}

void ControlBar::onPlaybackRateIndexChanged(const int index) {
    if (index < 0 || index >= speedCombo_->count()) {
        return;
    }
    emit playbackRateChanged(speedCombo_->itemData(index).toDouble());
}

QString ControlBar::formatTime(const double sec) {
    const int totalSeconds = std::max(0, static_cast<int>(std::llround(sec)));
    const int hours = totalSeconds / 3600;
    const int minutes = (totalSeconds % 3600) / 60;
    const int seconds = totalSeconds % 60;

    if (hours > 0) {
        return QString("%1:%2:%3").arg(hours, 2, 10, QChar('0')).arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10,
                                                                           QChar('0'));
    }

    return QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
}

void ControlBar::updateTimeLabel(const double currentSec) {
    timeLabel_->setText(QString("%1 / %2").arg(formatTime(currentSec), formatTime(durationSec_)));
}

void ControlBar::syncPlayPauseButtonText() {
    switch (playbackState_) {
    case playerlab::core::PlayerController::PlaybackState::Stopped:
        playPauseButton_->setText("Play");
        break;
    case playerlab::core::PlayerController::PlaybackState::Playing:
        playPauseButton_->setText("Pause");
        break;
    case playerlab::core::PlayerController::PlaybackState::Paused:
        playPauseButton_->setText("Resume");
        break;
    case playerlab::core::PlayerController::PlaybackState::Ended:
        playPauseButton_->setText("Replay");
        break;
    }
}
