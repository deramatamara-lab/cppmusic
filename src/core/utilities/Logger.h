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

private:
    static void writeToLog(const std::string& message);
};

} // namespace daw::core::utilities

