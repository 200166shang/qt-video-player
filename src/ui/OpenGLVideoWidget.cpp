#include "OpenGLVideoWidget.h"

#include <cmath>

#include "../render/RendererFactory.h"

OpenGLVideoWidget::OpenGLVideoWidget(QWidget* parent) : QOpenGLWidget(parent) {}

void OpenGLVideoWidget::setVideoFrame(const playerlab::core::VideoFrame& frame) {
    if (renderer_) {
        renderer_->setVideoFrame(frame);
        update();
    }
}

void OpenGLVideoWidget::initializeGL() {
    renderer_ = RendererFactory::create(RendererType::OpenGL);
    if (renderer_) {
        renderer_->initialize();
    }
}

void OpenGLVideoWidget::resizeGL(int w, int h) {
    if (renderer_) {
        const qreal dpr = devicePixelRatioF();
        const int pixelW = static_cast<int>(std::lround(static_cast<double>(w) * dpr));
        const int pixelH = static_cast<int>(std::lround(static_cast<double>(h) * dpr));
        renderer_->resize(pixelW, pixelH);
    }
}

void OpenGLVideoWidget::paintGL() {
    if (renderer_) {
        renderer_->render();
    }
}
