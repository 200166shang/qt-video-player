#include "TopBar.h"

#include <QHBoxLayout>
#include <QLabel>

TopBar::TopBar(QWidget* parent) : QWidget(parent) {
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 8, 12, 8);
    layout->addWidget(new QLabel("PlayerLab", this));
    layout->addStretch();
}
