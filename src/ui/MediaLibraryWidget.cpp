#include "MediaLibraryWidget.h"

#include <QLabel>
#include <QVBoxLayout>

MediaLibraryWidget::MediaLibraryWidget(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 8, 12, 8);
    layout->addWidget(new QLabel("Media Library (Placeholder)", this));
}
