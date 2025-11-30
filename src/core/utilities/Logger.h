#pragma once

#include <string>

namespace daw::core::utilities
{

/**
 * @brief Logging utility
 *
 * Thread-safe logging system. Never logs from audio thread.
 */
class Logger
{
public:
    enum class Level
    {
        Debug,
        Info,
        Warning,
        Error
    };

    static void log(Level level, const std::string& message);
    static void debug(const std::string& message);
    static void info(const std::string& message);
    static void warning(const std::string& message);
    static void error(const std::string& message);

    /**
     * @brief Enable/disable file logging
     * @param enable If true, logs will be written to file
     */
    static void setLogToFile(bool enable);

    /**
     * @brief Set minimum log level (filters out lower levels)
     * @param level Minimum level to log
     */
    static void setMinLogLevel(Level level);

    /**
     * @brief Get current minimum log level
     */
    static Level getMinLogLevel();

    /**
     * @brief Get current log file path
     * @return Log file path, or empty string if file logging disabled
     */
    static std::string getLogFilePath();

private:
    static void writeToLog(const std::string& message);
};

} // namespace daw::core::utilities

