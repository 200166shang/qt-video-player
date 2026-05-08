#pragma once

#include <QWidget>

class QPushButton;
class QSlider;

class ControlBar : public QWidget {
    Q_OBJECT

public:
    explicit ControlBar(QWidget* parent = nullptr);

signals:
    void openRequested();
    void pausedChanged(bool paused);
    void volumeChanged(float volume);
    void mutedChanged(bool muted);

private slots:
    void onOpenClicked();
    void onPlayClicked();
    void onProgressSliderChanged(int value);
    void onVolumeSliderChanged(int value);
    void onMuteClicked();

private:
    QPushButton* openButton_ = nullptr;
    QPushButton* playButton_ = nullptr;
    QPushButton* muteButton_ = nullptr;
    QSlider* progressSlider_ = nullptr;
    QSlider* volumeSlider_ = nullptr;
    bool paused_ = false;
    bool muted_ = false;
};
