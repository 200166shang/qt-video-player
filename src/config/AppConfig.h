#pragma once

#include <filesystem>
#include <string>

namespace playerlab::config {

struct AppConfigData {
    std::string logLevel{"info"};
    std::string preferredRenderer{"opengl"};
};

class AppConfig {
public:
    static AppConfigData loadOrDefault(const std::filesystem::path& path);
};

}  // namespace playerlab::config
