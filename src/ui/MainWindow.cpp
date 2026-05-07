#include "MainWindow.h"

#include <QSplitter>
#include <QVBoxLayout>

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

    auto* videoWidget = new OpenGLVideoWidget(this);
    auto* mediaLibrary = new MediaLibraryWidget(this);
    mediaLibrary->setFixedHeight(120);

    centerLayout->addWidget(videoWidget, 1);
    centerLayout->addWidget(mediaLibrary);

    auto* rightPanel = new MediaInfoPanel(this);
    rightPanel->setMinimumWidth(230);
    rightPanel->setMaximumWidth(350);

    mainSplitter->addWidget(sidebar);
    mainSplitter->addWidget(centerContainer);
    mainSplitter->addWidget(rightPanel);
    mainSplitter->setStretchFactor(0, 0);
    mainSplitter->setStretchFactor(1, 1);
    mainSplitter->setStretchFactor(2, 0);

    auto* controlBar = new ControlBar(this);
    controlBar->setFixedHeight(56);

    rootLayout->addWidget(topBar);
    rootLayout->addWidget(mainSplitter, 1);
    rootLayout->addWidget(controlBar);

    setStyleSheet(
        "QWidget { background: #1a1d21; color: #e6e8ea; }"
        "#topBar { background: #242a31; border-bottom: 1px solid #343d48; }"
        "QPushButton { background: #2d3640; border: 1px solid #3f4b59; padding: 6px 12px; }"
        "QPushButton:hover { background: #374351; }"
        "QSlider::groove:horizontal { height: 4px; background: #3a4654; }"
        "QSlider::handle:horizontal { background: #c9d7ea; width: 12px; margin: -6px 0; border-radius: 6px; }"
    );
}
