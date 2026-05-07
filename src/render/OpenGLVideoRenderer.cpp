#include "OpenGLVideoRenderer.h"

#include <QOpenGLContext>
#include <QOpenGLFunctions>

void OpenGLVideoRenderer::initialize() {
    auto* gl = QOpenGLContext::currentContext()->functions();
    gl->glClearColor(0.09F, 0.12F, 0.16F, 1.0F);
}

void OpenGLVideoRenderer::resize(int width, int height) {
    width_ = width;
    height_ = height;
}

void OpenGLVideoRenderer::render() {
    auto* gl = QOpenGLContext::currentContext()->functions();
    gl->glViewport(0, 0, width_, height_);
    gl->glClear(GL_COLOR_BUFFER_BIT);
}
