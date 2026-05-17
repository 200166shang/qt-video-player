#include "MainWindow.h"

#include <QFileDialog>
#include <QSplitter>
#include <QVBoxLayout>

#include "core/PlayerController.h"
#include "ControlBar.h"
#include "MediaInfoPanel.h"
#include "MediaLibraryWidget.h"
#include "OpenGLVideoWidget.h"
#include "Sidebar.h"
#include "TopBar.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("PlayerLab");
    resize(1280, 760);

    auto* central = new QWidget(this);
    setCentralWidget(central);

    auto* rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto* topBar = new TopBar(this);
    topBar->setObjectName("topBar");
    topBar->setFixedHeight(44);

    auto* mainSplitter = new QSplitter(Qt::Horizontal, this);

    auto* sidebar = new Sidebar(this);
    sidebar->setMinimumWidth(180);
    sidebar->setMaximumWidth(260);

    auto* centerContainer = new QWidget(this);
    auto* centerLayout = new QVBoxLayout(centerContainer);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    centerLayout->setSpacing(0);

    videoWidget_ = new OpenGLVideoWidget(this);
    auto* mediaLibrary = new MediaLibraryWidget(this);
    mediaLibrary->setFixedHeight(120);

    centerLayout->addWidget(videoWidget_, 1);
    centerLayout->addWidget(mediaLibrary);

    mediaInfoPanel_ = new MediaInfoPanel(this);
    mediaInfoPanel_->setMinimumWidth(230);
    mediaInfoPanel_->setMaximumWidth(350);

    mainSplitter->addWidget(sidebar);
    mainSplitter->addWidget(centerContainer);
    mainSplitter->addWidget(mediaInfoPanel_);
    mainSplitter->setStretchFactor(0, 0);
    mainSplitter->setStretchFactor(1, 1);
    mainSplitter->setStretchFactor(2, 0);

    controlBar_ = new ControlBar(this);
    controlBar_->setFixedHeight(56);

    rootLayout->addWidget(topBar);
    rootLayout->addWidget(mainSplitter, 1);
    rootLayout->addWidget(controlBar_);

    playerController_ = new playerlab::core::PlayerController(this);
    connect(controlBar_, &ControlBar::openRequested, this, &MainWindow::onOpenRequested);
    connect(controlBar_, &ControlBar::playPauseRequested, this, [this]() {
        if (playerController_ != nullptr) {
            playerController_->togglePlayPause();
        }
    });
    connect(controlBar_, &ControlBar::stopRequested, this, [this]() {
        if (playerController_ != nullptr) {
            playerController_->stop();
        }
    });
    connect(controlBar_, &ControlBar::seekRequested, this, [this](const double targetSec) {
        if (playerController_ != nullptr) {
            playerController_->seek(targetSec);
        }
    });
    connect(controlBar_, &ControlBar::playbackRateChanged, this, [this](const double rate) {
        if (playerController_ != nullptr) {
            playerController_->setPlaybackRate(rate);
        }
    });
    connect(controlBar_, &ControlBar::volumeChanged, this, [this](const float volume) {
        if (playerController_ != nullptr) {
            playerController_->setVolume(volume);
        }
    });
    connect(controlBar_, &ControlBar::mutedChanged, this, [this](const bool muted) {
        if (playerController_ != nullptr) {
            playerController_->setMuted(muted);
        }
    });
    connect(playerController_, &playerlab::core::PlayerController::mediaInfoChanged, this,
            [this](const playerlab::core::MediaInfo& info) {
                mediaInfoPanel_->setMediaInfo(info);
            });
    connect(playerController_, &playerlab::core::PlayerController::videoFrameReady, this,
            [this](const playerlab::core::VideoFrame& frame) {
                if (videoWidget_ != nullptr) {
                    videoWidget_->setVideoFrame(frame);
                }
            });
    connect(playerController_, &playerlab::core::PlayerController::openFailed, this,
            [this](const QString& error) {
                mediaInfoPanel_->setError(error);
            });
    connect(playerController_, &playerlab::core::PlayerController::playbackStateChanged, this,
            [this](const playerlab::core::PlayerController::PlaybackState state) {
                if (controlBar_ != nullptr) {
                    controlBar_->setPlaybackState(state);
                }
            });
    connect(playerController_, &playerlab::core::PlayerController::positionChanged, this,
            [this](const double currentSec, const double durationSec) {
                if (controlBar_ != nullptr) {
                    controlBar_->setProgress(currentSec, durationSec);
                }
            });
    connect(playerController_, &playerlab::core::PlayerController::durationChanged, this,
            [this](const double durationSec) {
                if (controlBar_ != nullptr) {
                    controlBar_->setDuration(durationSec);
                }
            });
    connect(playerController_, &playerlab::core::PlayerController::playbackRateChanged, this,
            [this](const double rate) {
                if (controlBar_ != nullptr) {
                    controlBar_->setPlaybackRate(rate);
                }
            });

    setStyleSheet(
        "QWidget { background: #1a1d21; color: #e6e8ea; }"
        "#topBar { background: #242a31; border-bottom: 1px solid #343d48; }"
        "QPushButton { background: #2d3640; border: 1px solid #3f4b59; padding: 6px 12px; }"
        "QPushButton:hover { background: #374351; }"
        "QSlider::groove:horizontal { height: 4px; background: #3a4654; }"
        "QSlider::handle:horizontal { background: #c9d7ea; width: 12px; margin: -6px 0; border-radius: 6px; }"
    );
}

void MainWindow::onOpenRequested() {
    const QString filter =
        "Media Files (*.mp4 *.MP4 *.mkv *.MKV *.mov *.MOV *.m4v *.M4V *.avi *.AVI);;All Files (*)";
    const QString filePath = QFileDialog::getOpenFileName(this, "Open Media File", QString(), filter);
    if (filePath.isEmpty()) {
        return;
    }
    playerController_->open(filePath);
}
