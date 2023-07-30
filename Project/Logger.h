#pragma once
#include <format>
#include <source_location>
#include <string_view>

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error,
    Fatal,
};

class Logger {
    LogLevel m_Priority{};
public:
    explicit Logger(LogLevel defaultLevel = LogLevel::Info);
    ~Logger();
    static Logger* DefaultLogger();

    void SetLogLevel(LogLevel level);
    void Write(LogLevel level, std::string_view message, std::source_location loc = std::source_location::current());
};

#define FATAL(message) Logger::DefaultLogger()->Write(LogLevel::Fatal, message)
#define ERROR(message) Logger::DefaultLogger()->Write(LogLevel::Error, message)
#define WARN(message) Logger::DefaultLogger()->Write(LogLevel::Warning, message)
#define INFO(message) Logger::DefaultLogger()->Write(LogLevel::Info, message)
#define DEBUG(message) Logger::DefaultLogger()->Write(LogLevel::Debug, message)

#define FATALF(fmt, ...) Logger::DefaultLogger()->Write(LogLevel::Fatal, std::format(fmt, ##__VA_ARGS__))
#define ERRORF(fmt, ...) Logger::DefaultLogger()->Write(LogLevel::Error, std::format(fmt, ##__VA_ARGS__))
#define WARNF(fmt, ...) Logger::DefaultLogger()->Write(LogLevel::Warning, std::format(fmt, ##__VA_ARGS__))
#define INFOF(fmt, ...) Logger::DefaultLogger()->Write(LogLevel::Info, std::format(fmt, ##__VA_ARGS__))
#define DEBUGF(fmt, ...) Logger::DefaultLogger()->Write(LogLevel::Debug, std::format(fmt, ##__VA_ARGS__))
