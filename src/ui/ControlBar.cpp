#include "ControlBar.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QSlider>

ControlBar::ControlBar(QWidget* parent) : QWidget(parent) {
    openButton_ = new QPushButton("Open", this);
    playButton_ = new QPushButton("Pause", this);
    muteButton_ = new QPushButton("Mute", this);
    progressSlider_ = new QSlider(Qt::Horizontal, this);
    progressSlider_->setRange(0, 1000);
    progressSlider_->setValue(0);
    volumeSlider_ = new QSlider(Qt::Horizontal, this);
    volumeSlider_->setRange(0, 100);
    volumeSlider_->setValue(100);
    volumeSlider_->setFixedWidth(140);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 8, 12, 8);
    layout->addWidget(openButton_);
    layout->addWidget(playButton_);
    layout->addWidget(progressSlider_, 1);
    layout->addWidget(muteButton_);
    layout->addWidget(volumeSlider_);

    connect(openButton_, &QPushButton::clicked, this, &ControlBar::onOpenClicked);
    connect(playButton_, &QPushButton::clicked, this, &ControlBar::onPlayClicked);
    connect(progressSlider_, &QSlider::valueChanged, this, &ControlBar::onProgressSliderChanged);
    connect(volumeSlider_, &QSlider::valueChanged, this, &ControlBar::onVolumeSliderChanged);
    connect(muteButton_, &QPushButton::clicked, this, &ControlBar::onMuteClicked);
}

void ControlBar::onOpenClicked() {
    emit openRequested();
}

void ControlBar::onPlayClicked() {
    paused_ = !paused_;
    playButton_->setText(paused_ ? "Resume" : "Pause");
    emit pausedChanged(paused_);
}

void ControlBar::onProgressSliderChanged(int value) {
    Q_UNUSED(value);
}

void ControlBar::onVolumeSliderChanged(const int value) {
    emit volumeChanged(static_cast<float>(value) / 100.0F);
}

void ControlBar::onMuteClicked() {
    muted_ = !muted_;
    muteButton_->setText(muted_ ? "Unmute" : "Mute");
    emit mutedChanged(muted_);
}
