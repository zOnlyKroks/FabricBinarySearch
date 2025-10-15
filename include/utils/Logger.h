#ifndef FABRICBINARYSEARCH_LOGGER_H
#define FABRICBINARYSEARCH_LOGGER_H

#include <string>
#include <fstream>
#include <mutex>
#include <memory>
#include <sstream>
#include <chrono>
#include <iomanip>

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

class Logger {
public:
    static Logger& getInstance();

    void setLogLevel(LogLevel level);
    void setLogFile(const std::string& filepath);
    void enableConsoleOutput(bool enable);
    void enableFileOutput(bool enable);

    void log(LogLevel level, const std::string& message,
             const std::string& file = "", int line = 0);

    void debug(const std::string& message, const std::string& file = "", int line = 0);
    void info(const std::string& message, const std::string& file = "", int line = 0);
    void warning(const std::string& message, const std::string& file = "", int line = 0);
    void error(const std::string& message, const std::string& file = "", int line = 0);
    void fatal(const std::string& message, const std::string& file = "", int line = 0);

    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

private:
    Logger();

    LogLevel currentLevel;
    bool consoleEnabled;
    bool fileEnabled;
    std::ofstream logFile;
    std::mutex logMutex;

    std::string levelToString(LogLevel level) const;
    std::string getCurrentTimestamp() const;
    void writeLog(const std::string& formattedMessage);
};

#define LOG_DEBUG(msg) Logger::getInstance().debug(msg, __FILE__, __LINE__)
#define LOG_INFO(msg) Logger::getInstance().info(msg, __FILE__, __LINE__)
#define LOG_WARNING(msg) Logger::getInstance().warning(msg, __FILE__, __LINE__)
#define LOG_ERROR(msg) Logger::getInstance().error(msg, __FILE__, __LINE__)
#define LOG_FATAL(msg) Logger::getInstance().fatal(msg, __FILE__, __LINE__)

#endif // FABRICBINARYSEARCH_LOGGER_H