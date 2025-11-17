#include "Logger.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace daw::core::utilities
{

void Logger::log(Level level, const std::string& message)
{
    std::string levelStr;
    switch (level)
    {
        case Level::Debug:   levelStr = "DEBUG"; break;
        case Level::Info:    levelStr = "INFO"; break;
        case Level::Warning: levelStr = "WARN"; break;
        case Level::Error:   levelStr = "ERROR"; break;
    }

    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    ss << " [" << levelStr << "] " << message;
    
    writeToLog(ss.str());
}

void Logger::debug(const std::string& message)
{
    log(Level::Debug, message);
}

void Logger::info(const std::string& message)
{
    log(Level::Info, message);
}

void Logger::warning(const std::string& message)
{
    log(Level::Warning, message);
}

void Logger::error(const std::string& message)
{
    log(Level::Error, message);
}

void Logger::writeToLog(const std::string& message)
{
    // In production, this would write to file or use JUCE's Logger
    std::cout << message << std::endl;
}

} // namespace daw::core::utilities

