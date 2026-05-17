#pragma once

#include <cstdint>
#include <memory>

#include <QOpenGLWidget>

#include "core/VideoFrame.h"
#include "../render/IVideoRenderer.h"

class OpenGLVideoWidget : public QOpenGLWidget {
    Q_OBJECT

public:
    explicit OpenGLVideoWidget(QWidget* parent = nullptr);
    void setVideoFrame(const playerlab::core::VideoFrame& frame);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private:
    std::unique_ptr<IVideoRenderer> renderer_;
    std::uint64_t debugFrameReceivedCount_ = 0;
};
