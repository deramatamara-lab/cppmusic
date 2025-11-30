#include "Logger.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <mutex>
#include <fstream>
#include <juce_core/juce_core.h>

namespace daw::core::utilities
{

namespace
{
    // Thread-safe logging state
    std::mutex logMutex;
    std::ofstream logFile;
    bool logToFile = false;
    std::string logFilePath;
    Logger::Level minLogLevel = Logger::Level::Info; // Default: show Info and above

    void initializeLogFile()
    {
        if (!logToFile || !logFilePath.empty())
            return;

        // Use JUCE's standard log location
        auto logDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                      .getChildFile("NeuroDAW")
                      .getChildFile("Logs");

        if (!logDir.exists())
            logDir.createDirectory();

        // Create log file with timestamp
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S");

        logFilePath = logDir.getChildFile("daw_" + ss.str() + ".log").getFullPathName().toStdString();
        logFile.open(logFilePath, std::ios::app);
    }

    void writeToFile(const std::string& message)
    {
        std::lock_guard<std::mutex> lock(logMutex);

        if (!logToFile)
            initializeLogFile();

        if (logFile.is_open())
        {
            logFile << message << std::endl;
            logFile.flush(); // Ensure immediate write
        }
    }
}

void Logger::log(Level level, const std::string& message)
{
    // Filter by minimum log level
    if (level < minLogLevel)
        return;

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
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    ss << "." << std::setfill('0') << std::setw(3) << ms.count();
    ss << " [" << levelStr << "] " << message;

    const std::string logMessage = ss.str();

    // Write to console (thread-safe via JUCE)
    juce::Logger::writeToLog(juce::String(logMessage));

    // Write to file if enabled
    if (logToFile)
    {
        writeToFile(logMessage);
    }
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
    // Production implementation: Use JUCE's Logger for cross-platform logging
    juce::Logger::writeToLog(juce::String(message));
}

void Logger::setLogToFile(bool enable)
{
    std::lock_guard<std::mutex> lock(logMutex);
    logToFile = enable;

    if (enable && !logFile.is_open())
    {
        initializeLogFile();
    }
    else if (!enable && logFile.is_open())
    {
        logFile.close();
        logFilePath.clear();
    }
}

void Logger::setMinLogLevel(Level level)
{
    std::lock_guard<std::mutex> lock(logMutex);
    minLogLevel = level;
}

Logger::Level Logger::getMinLogLevel()
{
    std::lock_guard<std::mutex> lock(logMutex);
    return minLogLevel;
}

std::string Logger::getLogFilePath()
{
    std::lock_guard<std::mutex> lock(logMutex);
    return logFilePath;
}

} // namespace daw::core::utilities
