#include "config/AppConfig.h"

#include <filesystem>

#include <yaml-cpp/yaml.h>

#include "utils/Logger.h"

namespace playerlab::config {

AppConfigData AppConfig::loadOrDefault(const std::filesystem::path& path) {
    AppConfigData config;

    if (!std::filesystem::exists(path)) {
        LOG_WARN("AppConfig file not found: {}. Using defaults.", path.string());
        return config;
    }

    try {
        const YAML::Node root = YAML::LoadFile(path.string());

        if (root["log_level"]) {
            config.logLevel = root["log_level"].as<std::string>();
        }

        if (root["preferred_renderer"]) {
            config.preferredRenderer = root["preferred_renderer"].as<std::string>();
        }
    } catch (const YAML::BadFile& e) {
        LOG_ERROR("Failed to open AppConfig YAML file {}: {}. Using defaults.", path.string(), e.what());
        return AppConfigData{};
    } catch (const YAML::ParserException& e) {
        LOG_ERROR("Failed to parse AppConfig YAML file {}: {}. Using defaults.", path.string(), e.what());
        return AppConfigData{};
    } catch (const YAML::BadConversion& e) {
        LOG_ERROR("Invalid type in AppConfig YAML file {}: {}. Using defaults.", path.string(), e.what());
        return AppConfigData{};
    }

    return config;
}

}  // namespace playerlab::config
