#pragma once

#include <QLabel>
#include <QWidget>

namespace playerlab::core {
struct MediaInfo;
}

class MediaInfoPanel : public QWidget {
    Q_OBJECT

public:
    explicit MediaInfoPanel(QWidget* parent = nullptr);

    void clearInfo();
    void setMediaInfo(const playerlab::core::MediaInfo& info);
    void setError(const QString& message);

private:
    QLabel* fileValue_ = nullptr;
    QLabel* formatValue_ = nullptr;
    QLabel* durationValue_ = nullptr;
    QLabel* videoValue_ = nullptr;
    QLabel* audioValue_ = nullptr;
    QLabel* errorValue_ = nullptr;
};
