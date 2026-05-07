#include "MediaInfoPanel.h"

#include <QLabel>
#include <QVBoxLayout>

MediaInfoPanel::MediaInfoPanel(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->addWidget(new QLabel("Media Info", this));
    layout->addWidget(new QLabel("Duration: --:--", this));
    layout->addWidget(new QLabel("Video: --", this));
    layout->addWidget(new QLabel("Audio: --", this));
    layout->addStretch();
}
