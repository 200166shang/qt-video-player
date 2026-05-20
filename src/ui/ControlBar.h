#pragma once

#include <QWidget>

#include "core/PlayerController.h"

class QPushButton;
class QSlider;
class QLabel;
class QComboBox;

class ControlBar : public QWidget {
    Q_OBJECT

public:
    explicit ControlBar(QWidget* parent = nullptr);

    void setPlaybackState(playerlab::core::PlaybackState state);
    void setProgress(double currentSec, double durationSec);
    void setDuration(double durationSec);
    void setPlaybackRate(double rate);

signals:
    void openRequested();
    void playPauseRequested();
    void stopRequested();
    void seekRequested(double targetSec);
    void playbackRateChanged(double rate);
    void volumeChanged(float volume);
    void mutedChanged(bool muted);

private slots:
    void onOpenClicked();
    void onPlayPauseClicked();
    void onStopClicked();
    void onProgressPressed();
    void onProgressReleased();
    void onProgressValueChanged(int value);
    void onVolumeSliderChanged(int value);
    void onMuteClicked();
    void onPlaybackRateIndexChanged(int index);

private:
    static QString formatTime(double sec);
    void updateTimeLabel(double currentSec);
    void syncPlayPauseButtonText();

    QPushButton* openButton_ = nullptr;
    QPushButton* playPauseButton_ = nullptr;
    QPushButton* stopButton_ = nullptr;
    QPushButton* muteButton_ = nullptr;
    QSlider* progressSlider_ = nullptr;
    QSlider* volumeSlider_ = nullptr;
    QLabel* timeLabel_ = nullptr;
    QComboBox* speedCombo_ = nullptr;

    playerlab::core::PlaybackState playbackState_ = playerlab::core::PlaybackState::Stopped;
    bool muted_ = false;
    bool seeking_ = false;
    double durationSec_ = 0.0;
    double currentSec_ = 0.0;
};
