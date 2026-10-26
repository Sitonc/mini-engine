#pragma once
#include <cstdio>
#include <format>
#include <utility>

enum class LogLevel { Info, Warn, Error };

template <typename... Args>
void Log(LogLevel level, std::format_string<Args...> fmt, Args&&... args)
{
    const char* tag = level == LogLevel::Info ? "INFO"
        : level == LogLevel::Warn ? "WARN" : "ERROR";
    std::string msg = std::format(fmt, std::forward<Args>(args)...);
    std::fprintf(stderr, "[%s] %s\n", tag, msg.c_str());
}

#define LOG_INFO(...)  Log(LogLevel::Info,  __VA_ARGS__)
#define LOG_WARN(...)  Log(LogLevel::Warn,  __VA_ARGS__)
#define LOG_ERROR(...) Log(LogLevel::Error, __VA_ARGS__)