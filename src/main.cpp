#include <QApplication>

#include <cstdlib>
#include <filesystem>
#include <string_view>

#include "config/AppConfig.h"
#include "ffmpeg/FFmpegProbe.h"
#include "ui/MainWindow.h"
#include "utils/Logger.h"
#include "utils/StbVersion.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    playerlab::utils::Logger::init();

    bool pipelineDebug = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string_view(argv[i]) == "--debug-pipeline") {
            pipelineDebug = true;
            break;
        }
    }
    if (!pipelineDebug) {
        const char* env = std::getenv("PLAYERLAB_DEBUG_PIPELINE");
        pipelineDebug = env != nullptr && std::string_view(env) == "1";
    }
    playerlab::utils::Logger::setPipelineDebugEnabled(pipelineDebug);

    const auto config = playerlab::config::AppConfig::loadOrDefault(
        std::filesystem::path("config/default.yaml"));
    playerlab::utils::Logger::setLevel(config.logLevel);
    if (pipelineDebug) {
        playerlab::utils::Logger::setLevel("trace");
    }

    LOG_INFO("PlayerLab startup");
    LOG_INFO("Pipeline debug: {}", pipelineDebug ? "ON" : "OFF");
    LOG_INFO("Renderer: {}", config.preferredRenderer);
    LOG_INFO("FFmpeg probe: {}", playerlab::ffmpeg::FFmpegProbe::versionString());
    LOG_INFO("STB probe: {}", playerlab::utils::StbVersion::name());

    MainWindow window;
    window.show();

    return app.exec();
}
