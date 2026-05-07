#pragma once

#include <memory>

#include <QOpenGLWidget>

#include "../render/IVideoRenderer.h"

class OpenGLVideoWidget : public QOpenGLWidget {
    Q_OBJECT

public:
    explicit OpenGLVideoWidget(QWidget* parent = nullptr);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private:
    std::unique_ptr<IVideoRenderer> renderer_;
};
