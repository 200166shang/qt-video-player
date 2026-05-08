#pragma once

#include <memory>

#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>

#include "IVideoRenderer.h"

class OpenGLVideoRenderer : public IVideoRenderer {
public:
    void initialize() override;
    void resize(int width, int height) override;
    void setVideoFrame(const playerlab::core::VideoFrame& frame) override;
    void render() override;

private:
    void ensureTextures(const playerlab::core::VideoFrame& frame);

    int viewportWidth_ = 0;
    int viewportHeight_ = 0;
    playerlab::core::VideoFrame currentFrame_;

    QOpenGLShaderProgram shader_;
    QOpenGLVertexArrayObject vao_;
    QOpenGLBuffer vertexBuffer_{QOpenGLBuffer::VertexBuffer};
    std::unique_ptr<QOpenGLTexture> yTexture_;
    std::unique_ptr<QOpenGLTexture> uTexture_;
    std::unique_ptr<QOpenGLTexture> vTexture_;
    bool glReady_ = false;
};
