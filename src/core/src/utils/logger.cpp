// clang-format off
#include "utils/warning_push.h"
#include "utils/warning_ignore_general.h"
#include "logger.h"
#include "utils/warning_pop.h"
// clang-format on

namespace rt
{
void Logger::SetLogLevel(RRLogLevel log_level) const
{
    switch (log_level)
    {
    case RR_LOG_LEVEL_DEBUG:
        logger_->set_level(spdlog::level::debug);
        break;
    case RR_LOG_LEVEL_INFO:
        logger_->set_level(spdlog::level::info);
        break;
    case RR_LOG_LEVEL_WARN:
        logger_->set_level(spdlog::level::warn);
        break;
    case RR_LOG_LEVEL_ERROR:
        logger_->set_level(spdlog::level::err);
        break;
    case RR_LOG_LEVEL_OFF:
        logger_->set_level(spdlog::level::off);
        break;
    default:
        throw std::runtime_error("Incorrect log_level");
    }
}

void Logger::SetFileLogger(char const* filename)
{
    auto file_logger = std::string(LoggerName) + ": " + filename;
    logger_          = spdlog::get(file_logger);
    if (!logger_)
    {
        logger_ = spdlog::basic_logger_mt(file_logger, filename);
    }
}

void Logger::SetConsoleLogger() { logger_ = spdlog::get(LoggerName); }

}  // namespace rt