#include "ControlBar.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QSlider>

ControlBar::ControlBar(QWidget* parent) : QWidget(parent) {
    openButton_ = new QPushButton("Open", this);
    playButton_ = new QPushButton("Play", this);
    progressSlider_ = new QSlider(Qt::Horizontal, this);
    progressSlider_->setRange(0, 1000);
    progressSlider_->setValue(0);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 8, 12, 8);
    layout->addWidget(openButton_);
    layout->addWidget(playButton_);
    layout->addWidget(progressSlider_, 1);

    connect(openButton_, &QPushButton::clicked, this, &ControlBar::onOpenClicked);
    connect(playButton_, &QPushButton::clicked, this, &ControlBar::onPlayClicked);
    connect(progressSlider_, &QSlider::valueChanged, this, &ControlBar::onSliderChanged);
}

void ControlBar::onOpenClicked() {}

void ControlBar::onPlayClicked() {}

void ControlBar::onSliderChanged(int value) {
    Q_UNUSED(value);
}
