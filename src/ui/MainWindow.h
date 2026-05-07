#pragma once

#include <QMainWindow>

class ControlBar;
class MediaInfoPanel;

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
    playerlab::core::PlayerController* playerController_ = nullptr;
};
