#include "Logger.h"
#include <iostream>
#include <memory>
#include <map>

const std::map<LogLevel, int> ColorCodes {
        {LogLevel::Fatal, 31},
        {LogLevel::Error, 31},
        {LogLevel::Warning, 33},
        {LogLevel::Info, 32},
        {LogLevel::Debug, 37},
};

std::unique_ptr<Logger> g_DefaultLogger;

Logger *Logger::DefaultLogger() {
    if (g_DefaultLogger) {
        return g_DefaultLogger.get();
    }
    g_DefaultLogger = std::make_unique<Logger>();
    return g_DefaultLogger.get();
}

Logger::Logger(LogLevel defaultLevel): m_Priority(defaultLevel) {

}

Logger::~Logger() = default;

void Logger::SetLogLevel(LogLevel level) {
    m_Priority = level;
}

void Logger::Write(LogLevel level, std::string_view message, std::source_location loc) {
    const char* prefixes[] = {"[ DEBUG ]", "[ INFO  ]", "[WARNING]", "[ ERROR ]", "[ FATAL ]"};

    if (level >= m_Priority) {
        std::cout <<  std::format("\033[1;{}m{}: {}\033[0m\n", ColorCodes.at(level), prefixes[static_cast<int>(level)], message);
        if (level == LogLevel::Fatal) {
            // Do something special?
        }
    }
}
