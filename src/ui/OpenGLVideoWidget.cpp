#include "OpenGLVideoWidget.h"

#include "../render/RendererFactory.h"

OpenGLVideoWidget::OpenGLVideoWidget(QWidget* parent) : QOpenGLWidget(parent) {}

void OpenGLVideoWidget::initializeGL() {
    renderer_ = RendererFactory::create(RendererType::OpenGL);
    if (renderer_) {
        renderer_->initialize();
    }
}

void OpenGLVideoWidget::resizeGL(int w, int h) {
    if (renderer_) {
        renderer_->resize(w, h);
    }
}

void OpenGLVideoWidget::paintGL() {
    if (renderer_) {
        renderer_->render();
    }
}
