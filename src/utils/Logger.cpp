#include "utils/Logger.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace playerlab::utils {

namespace {
constexpr const char* kLoggerName = "playerlab";
}

void Logger::init() {
    if (spdlog::get(kLoggerName) != nullptr) {
        return;
    }

    auto logger = spdlog::stdout_color_mt(kLoggerName);
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [tid %t] [%s:%# %!] %v");
    logger->set_level(spdlog::level::info);
    logger->flush_on(spdlog::level::warn);

    spdlog::set_default_logger(logger);
    spdlog::set_level(spdlog::level::info);
}

std::shared_ptr<spdlog::logger> Logger::get() {
    auto logger = spdlog::get(kLoggerName);
    if (logger == nullptr) {
        init();
        logger = spdlog::get(kLoggerName);
    }
    return logger;
}

void Logger::setLevel(const std::string& level) {
    auto logger = get();
    const auto parsed = spdlog::level::from_str(level);
    logger->set_level(parsed);
    spdlog::set_level(parsed);
}

}  // namespace playerlab::utils
