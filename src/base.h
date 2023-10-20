#pragma once

#include <vector>
#include <array>
#include <format>
#include <source_location>
#include <string_view>
#include <filesystem>
#include <iostream>
#include <chrono>
#include <cassert>
#include <cstring>

namespace ez
{

    enum class LogLevel
    {
        INFO,
        WARN,
        ERROR
    };

    const char *to_string(LogLevel level);
    std::string to_string(const std::chrono::system_clock::time_point& tp);
    std::string to_string(const std::source_location &source);

    template <LogLevel level, typename... TArgs>
    struct log
    {
        log(std::string_view format, TArgs&&... args, std::source_location location = std::source_location::current())
        {
            std::cout << std::format("[{}] {} | {} | {}\n",
                                     to_string(level),
                                     to_string(std::chrono::system_clock::now()),
                                     to_string(location),
                                     std::vformat(format, std::make_format_args(std::forward<TArgs>(args)...)));
        }
    };

    template <LogLevel level, typename... TArgs>
    log(std::string_view, TArgs&&...) -> log<level, TArgs...>;
    
    template <typename... TArgs>
    using log_info = log<LogLevel::INFO, TArgs...>;
    
    template <typename... TArgs>
    using log_error = log<LogLevel::ERROR, TArgs...>;

    template <typename... TArgs>
    using log_warn = log<LogLevel::WARN, TArgs...>;
    
}