#include <QApplication>

#include <filesystem>

#include "config/AppConfig.h"
#include "ffmpeg/FFmpegProbe.h"
#include "ui/MainWindow.h"
#include "utils/Logger.h"
#include "utils/StbVersion.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    playerlab::utils::Logger::init();

    const auto config = playerlab::config::AppConfig::loadOrDefault(
        std::filesystem::path("config/default.yaml"));
    playerlab::utils::Logger::setLevel(config.logLevel);

    LOG_INFO("PlayerLab startup");
    LOG_INFO("Renderer: {}", config.preferredRenderer);
    LOG_INFO("FFmpeg probe: {}", playerlab::ffmpeg::FFmpegProbe::versionString());
    LOG_INFO("STB probe: {}", playerlab::utils::StbVersion::name());

    MainWindow window;
    window.show();

    return app.exec();
}
