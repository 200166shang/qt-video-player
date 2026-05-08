#include "OpenGLVideoRenderer.h"

#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QSurfaceFormat>

namespace {

constexpr const char* kVertexShader330 = R"(
#version 330
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTex;
out vec2 vTex;
void main() {
    gl_Position = vec4(aPos.xy, 0.0, 1.0);
    vTex = aTex;
}
)";

constexpr const char* kFragmentShader330 = R"(
#version 330
in vec2 vTex;
out vec4 FragColor;
uniform sampler2D texY;
uniform sampler2D texU;
uniform sampler2D texV;
void main() {
    float y = texture(texY, vTex).r;
    float u = texture(texU, vTex).r - 0.5;
    float v = texture(texV, vTex).r - 0.5;

    float r = y + 1.402 * v;
    float g = y - 0.344136 * u - 0.714136 * v;
    float b = y + 1.772 * u;
    FragColor = vec4(r, g, b, 1.0);
}
)";

constexpr const char* kVertexShader120 = R"(
#version 120
attribute vec2 aPos;
attribute vec2 aTex;
varying vec2 vTex;
void main() {
    gl_Position = vec4(aPos.xy, 0.0, 1.0);
    vTex = aTex;
}
)";

constexpr const char* kFragmentShader120 = R"(
#version 120
varying vec2 vTex;
uniform sampler2D texY;
uniform sampler2D texU;
uniform sampler2D texV;
void main() {
    float y = texture2D(texY, vTex).r;
    float u = texture2D(texU, vTex).r - 0.5;
    float v = texture2D(texV, vTex).r - 0.5;

    float r = y + 1.402 * v;
    float g = y - 0.344136 * u - 0.714136 * v;
    float b = y + 1.772 * u;
    gl_FragColor = vec4(r, g, b, 1.0);
}
)";

constexpr const char* kVertexShaderEs100 = R"(
attribute vec2 aPos;
attribute vec2 aTex;
varying vec2 vTex;
void main() {
    gl_Position = vec4(aPos.xy, 0.0, 1.0);
    vTex = aTex;
}
)";

constexpr const char* kFragmentShaderEs100 = R"(
precision mediump float;
varying vec2 vTex;
uniform sampler2D texY;
uniform sampler2D texU;
uniform sampler2D texV;
void main() {
    float y = texture2D(texY, vTex).r;
    float u = texture2D(texU, vTex).r - 0.5;
    float v = texture2D(texV, vTex).r - 0.5;

    float r = y + 1.402 * v;
    float g = y - 0.344136 * u - 0.714136 * v;
    float b = y + 1.772 * u;
    gl_FragColor = vec4(r, g, b, 1.0);
}
)";

}  // namespace

void OpenGLVideoRenderer::initialize() {
    auto* context = QOpenGLContext::currentContext();
    auto* gl = context->functions();
    gl->glClearColor(0.09F, 0.12F, 0.16F, 1.0F);

    const bool isEs = context->isOpenGLES();
    const QSurfaceFormat fmt = context->format();
    const bool useGl330 = !isEs && (fmt.majorVersion() >= 3);

    const char* vertexSource = useGl330 ? kVertexShader330 : (isEs ? kVertexShaderEs100 : kVertexShader120);
    const char* fragmentSource =
        useGl330 ? kFragmentShader330 : (isEs ? kFragmentShaderEs100 : kFragmentShader120);

    if (!shader_.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexSource)) {
        return;
    }
    if (!shader_.addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentSource)) {
        return;
    }
    if (!shader_.link()) {
        return;
    }

    constexpr float vertices[] = {
        -1.0f, -1.0f, 0.0f, 1.0f,
         1.0f, -1.0f, 1.0f, 1.0f,
         1.0f,  1.0f, 1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f, 0.0f,
    };

    vao_.create();
    vao_.bind();
    vertexBuffer_.create();
    vertexBuffer_.bind();
    vertexBuffer_.allocate(vertices, sizeof(vertices));

    shader_.bind();
    shader_.enableAttributeArray("aPos");
    shader_.setAttributeBuffer("aPos", GL_FLOAT, 0, 2, 4 * static_cast<int>(sizeof(float)));
    shader_.enableAttributeArray("aTex");
    shader_.setAttributeBuffer("aTex", GL_FLOAT, 2 * static_cast<int>(sizeof(float)), 2,
                               4 * static_cast<int>(sizeof(float)));

    vertexBuffer_.release();
    vao_.release();
    shader_.release();

    glReady_ = true;
}

void OpenGLVideoRenderer::resize(int width, int height) {
    viewportWidth_ = width;
    viewportHeight_ = height;
}

void OpenGLVideoRenderer::setVideoFrame(const playerlab::core::VideoFrame& frame) {
    currentFrame_ = frame;
}

void OpenGLVideoRenderer::render() {
    auto* gl = QOpenGLContext::currentContext()->functions();
    gl->glClear(GL_COLOR_BUFFER_BIT);

    if (!glReady_ || !currentFrame_.isValidYuv420p()) {
        gl->glViewport(0, 0, viewportWidth_, viewportHeight_);
        return;
    }

    ensureTextures(currentFrame_);
    if (!yTexture_ || !uTexture_ || !vTexture_) {
        return;
    }

    yTexture_->bind(0);
    uTexture_->bind(1);
    vTexture_->bind(2);

    yTexture_->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, currentFrame_.planes[0].data(), nullptr);
    uTexture_->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, currentFrame_.planes[1].data(), nullptr);
    vTexture_->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, currentFrame_.planes[2].data(), nullptr);

    const float videoAspect = static_cast<float>(currentFrame_.width) / static_cast<float>(currentFrame_.height);
    const float viewAspect = viewportHeight_ > 0 ? static_cast<float>(viewportWidth_) / static_cast<float>(viewportHeight_) : 1.0F;

    int vpX = 0;
    int vpY = 0;
    int vpW = viewportWidth_;
    int vpH = viewportHeight_;

    if (viewAspect > videoAspect) {
        vpW = static_cast<int>(viewportHeight_ * videoAspect);
        vpX = (viewportWidth_ - vpW) / 2;
    } else {
        vpH = static_cast<int>(viewportWidth_ / videoAspect);
        vpY = (viewportHeight_ - vpH) / 2;
    }
    gl->glViewport(vpX, vpY, vpW, vpH);

    shader_.bind();
    shader_.setUniformValue("texY", 0);
    shader_.setUniformValue("texU", 1);
    shader_.setUniformValue("texV", 2);

    vao_.bind();
    gl->glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    vao_.release();
    shader_.release();
}

void OpenGLVideoRenderer::ensureTextures(const playerlab::core::VideoFrame& frame) {
    const bool needsRecreate = !yTexture_ || yTexture_->width() != frame.linesize[0] ||
                               yTexture_->height() != frame.height || uTexture_->width() != frame.linesize[1] ||
                               uTexture_->height() != frame.height / 2 || vTexture_->width() != frame.linesize[2] ||
                               vTexture_->height() != frame.height / 2;

    if (!needsRecreate) {
        return;
    }

    yTexture_.reset();
    uTexture_.reset();
    vTexture_.reset();

    yTexture_ = std::make_unique<QOpenGLTexture>(QOpenGLTexture::Target2D);
    yTexture_->setFormat(QOpenGLTexture::R8_UNorm);
    yTexture_->setWrapMode(QOpenGLTexture::ClampToEdge);
    yTexture_->setMinificationFilter(QOpenGLTexture::Linear);
    yTexture_->setMagnificationFilter(QOpenGLTexture::Linear);
    yTexture_->setSize(frame.linesize[0], frame.height);
    yTexture_->allocateStorage(QOpenGLTexture::Red, QOpenGLTexture::UInt8);

    uTexture_ = std::make_unique<QOpenGLTexture>(QOpenGLTexture::Target2D);
    uTexture_->setFormat(QOpenGLTexture::R8_UNorm);
    uTexture_->setWrapMode(QOpenGLTexture::ClampToEdge);
    uTexture_->setMinificationFilter(QOpenGLTexture::Linear);
    uTexture_->setMagnificationFilter(QOpenGLTexture::Linear);
    uTexture_->setSize(frame.linesize[1], frame.height / 2);
    uTexture_->allocateStorage(QOpenGLTexture::Red, QOpenGLTexture::UInt8);

    vTexture_ = std::make_unique<QOpenGLTexture>(QOpenGLTexture::Target2D);
    vTexture_->setFormat(QOpenGLTexture::R8_UNorm);
    vTexture_->setWrapMode(QOpenGLTexture::ClampToEdge);
    vTexture_->setMinificationFilter(QOpenGLTexture::Linear);
    vTexture_->setMagnificationFilter(QOpenGLTexture::Linear);
    vTexture_->setSize(frame.linesize[2], frame.height / 2);
    vTexture_->allocateStorage(QOpenGLTexture::Red, QOpenGLTexture::UInt8);
}
