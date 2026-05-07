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

private slots:
    void onOpenClicked();
    void onPlayClicked();
    void onSliderChanged(int value);

private:
    QPushButton* openButton_ = nullptr;
    QPushButton* playButton_ = nullptr;
    QSlider* progressSlider_ = nullptr;
};
