#include "Sidebar.h"

#include <QLabel>
#include <QVBoxLayout>

Sidebar::Sidebar(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->addWidget(new QLabel("Sidebar", this));
    layout->addStretch();
}
