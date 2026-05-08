#pragma once

#include <QMainWindow>

class ControlBar;
class MediaInfoPanel;
class OpenGLVideoWidget;

namespace playerlab::core {
class PlayerController;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void onOpenRequested();

private:
    ControlBar* controlBar_ = nullptr;
    MediaInfoPanel* mediaInfoPanel_ = nullptr;
    OpenGLVideoWidget* videoWidget_ = nullptr;
    playerlab::core::PlayerController* playerController_ = nullptr;
};
