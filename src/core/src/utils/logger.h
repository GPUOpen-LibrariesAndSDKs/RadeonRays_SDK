/**********************************************************************
Copyright (c) 2020 Advanced Micro Devices, Inc. All rights reserved.
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
********************************************************************/
#pragma once
#include <utility>
// clang-format off
#include "utils/warning_push.h"
#include "utils/warning_ignore_general.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"
#include "radeonrays.h"
#include "utils/warning_pop.h"
// clang-format on

namespace rt
{
class Logger
{
    static constexpr char const* LoggerName = "RR logger";

public:
    Logger(const Logger&)  = delete;
    Logger(Logger&&)       = delete;
    Logger&        operator=(const Logger&) = delete;
    Logger&        operator=(Logger&&) = delete;
    static Logger& Get()
    {
        static Logger logger;
        return logger;
    }

    template <typename... Args>
    void Info(Args&&... args);

    template <typename... Args>
    void Warn(Args&&... args);

    template <typename... Args>
    void Error(Args&&... args);

    template <typename... Args>
    void Debug(Args&&... args);

    template <typename... Args>
    void Trace(Args&&... args);

    void SetLogLevel(RRLogLevel log_level) const;
    void SetFileLogger(char const* filename);
    void SetConsoleLogger();

private:
    Logger()
    {
        logger_ = spdlog::stdout_color_mt(LoggerName);
#ifdef NDEBUG
        logger_->set_level(spdlog::level::info);
#else
        logger_->set_level(spdlog::level::debug);
#endif
    }
    ~Logger() { spdlog::shutdown(); }
    std::shared_ptr<spdlog::logger> logger_;
};

template <typename... Args>
void Logger::Info(Args&&... args)
{
    logger_->info(std::forward<Args>(args)...);
}

template <typename... Args>
void Logger::Warn(Args&&... args)
{
    logger_->warn(std::forward<Args>(args)...);
}

template <typename... Args>
void Logger::Error(Args&&... args)
{
    logger_->error(std::forward<Args>(args)...);
}

template <typename... Args>
void Logger::Debug(Args&&... args)
{
    logger_->debug(std::forward<Args>(args)...);
}

template <typename... Args>
void Logger::Trace(Args&&... args)
{
    logger_->trace(std::forward<Args>(args)...);
}

}  // namespace rt