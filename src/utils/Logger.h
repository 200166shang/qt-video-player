#pragma once

#include <memory>
#include <string>

#include <spdlog/common.h>
#include <spdlog/logger.h>
#include <spdlog/spdlog.h>

namespace playerlab::utils {

class Logger {
public:
    static void init();
    static std::shared_ptr<spdlog::logger> get();
    static void setLevel(const std::string& level);
};

}  // namespace playerlab::utils

#define LOG_TRACE(...) SPDLOG_LOGGER_TRACE(playerlab::utils::Logger::get(), __VA_ARGS__)
#define LOG_DEBUG(...) SPDLOG_LOGGER_DEBUG(playerlab::utils::Logger::get(), __VA_ARGS__)
#define LOG_INFO(...) SPDLOG_LOGGER_INFO(playerlab::utils::Logger::get(), __VA_ARGS__)
#define LOG_WARN(...) SPDLOG_LOGGER_WARN(playerlab::utils::Logger::get(), __VA_ARGS__)
#define LOG_ERROR(...) SPDLOG_LOGGER_ERROR(playerlab::utils::Logger::get(), __VA_ARGS__)
#define LOG_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(playerlab::utils::Logger::get(), __VA_ARGS__)
