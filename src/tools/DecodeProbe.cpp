#include <QGuiApplication>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLShader>
#include <QOpenGLShaderProgram>
#include <QSurfaceFormat>

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "core/MediaSource.h"
#include "core/VideoFrame.h"
#include "ffmpeg/FFmpegGlobal.h"
#include "ffmpeg/FFmpegVideoDecoder.h"

namespace {

void probeOpenGL() {
    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setProfile(QSurfaceFormat::NoProfile);
    format.setVersion(2, 1);
    QSurfaceFormat::setDefaultFormat(format);

    QOpenGLContext context;
    context.setFormat(format);
    if (!context.create()) {
        std::cout << "[gl] create context failed\n";
        return;
    }

    QOffscreenSurface surface;
    surface.setFormat(context.format());
    surface.create();

    if (!context.makeCurrent(&surface)) {
        std::cout << "[gl] makeCurrent failed\n";
        return;
    }

    const QSurfaceFormat actual = context.format();
    std::cout << "[gl] isGLES=" << (context.isOpenGLES() ? "true" : "false")
              << " version=" << actual.majorVersion() << "." << actual.minorVersion() << "\n";

    QOpenGLShaderProgram program;
    const bool isEs = context.isOpenGLES();
    const bool use330 = !isEs && actual.majorVersion() >= 3;

    const char* vs330 = "#version 330\nlayout(location=0) in vec2 aPos; void main(){ gl_Position=vec4(aPos,0.0,1.0);}";
    const char* fs330 = "#version 330\nout vec4 FragColor; void main(){ FragColor=vec4(1.0);}";

    const char* vs120 = "#version 120\nattribute vec2 aPos; void main(){ gl_Position=vec4(aPos,0.0,1.0);}";
    const char* fs120 = "#version 120\nvoid main(){ gl_FragColor=vec4(1.0);}";

    const char* vs100 = "attribute vec2 aPos; void main(){ gl_Position=vec4(aPos,0.0,1.0);}";
    const char* fs100 = "precision mediump float; void main(){ gl_FragColor=vec4(1.0);}";

    const char* vs = use330 ? vs330 : (isEs ? vs100 : vs120);
    const char* fs = use330 ? fs330 : (isEs ? fs100 : fs120);

    const bool vOk = program.addShaderFromSourceCode(QOpenGLShader::Vertex, vs);
    const bool fOk = program.addShaderFromSourceCode(QOpenGLShader::Fragment, fs);
    const bool lOk = vOk && fOk && program.link();

    std::cout << "[gl] shader path=" << (use330 ? "330" : (isEs ? "es100" : "120"))
              << " vertex=" << (vOk ? "ok" : "fail")
              << " fragment=" << (fOk ? "ok" : "fail")
              << " link=" << (lOk ? "ok" : "fail") << "\n";
    if (!vOk || !fOk || !lOk) {
        std::cout << "[gl] shader log:\n" << program.log().toStdString() << "\n";
    }

    context.doneCurrent();
}

void probeDecode(const std::string& uri) {
    playerlab::ffmpeg::FFmpegGlobal::initialize();

    playerlab::ffmpeg::FFmpegVideoDecoder decoder;
    playerlab::core::MediaSource source{.uri = uri};
    std::string error;
    if (!decoder.open(source, error)) {
        std::cout << "[decode] open failed: " << error << "\n";
        return;
    }

    std::cout << "[decode] open ok\n";
    int frames = 0;
    const auto start = std::chrono::steady_clock::now();
    while (frames < 10) {
        playerlab::core::VideoFrame frame;
        if (decoder.tryPopFrame(frame)) {
            ++frames;
            std::cout << "[decode] frame#" << frames << " " << frame.width << "x" << frame.height
                      << " yStride=" << frame.linesize[0] << " uStride=" << frame.linesize[1]
                      << " vStride=" << frame.linesize[2] << " pts=" << frame.ptsSec << "\n";
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        if (std::chrono::steady_clock::now() - start > std::chrono::seconds(3)) {
            break;
        }
    }

    decoder.stop();
    std::cout << "[decode] total frames collected=" << frames << "\n";
}

}  // namespace

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);

    const std::string input = argc > 1 ? argv[1] : "testdata/sample_640x360.mp4";
    std::cout << "[probe] input=" << input << "\n";

    probeOpenGL();
    probeDecode(input);
    return 0;
}
