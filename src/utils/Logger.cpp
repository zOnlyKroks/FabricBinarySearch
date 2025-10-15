#include "Logger.h"
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

Logger::Logger()
    : currentLevel(LogLevel::INFO),
      consoleEnabled(true),
      fileEnabled(false) {
}

Logger::~Logger() {
    if (logFile.is_open()) {
        logFile.close();
    }
}

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

void Logger::setLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(logMutex);
    currentLevel = level;
}

void Logger::setLogFile(const std::string& filepath) {
    std::lock_guard<std::mutex> lock(logMutex);

    if (logFile.is_open()) {
        logFile.close();
    }

    fs::path logPath(filepath);
    fs::path logDir = logPath.parent_path();

    if (!logDir.empty() && !fs::exists(logDir)) {
        try {
            fs::create_directories(logDir);
        } catch (const fs::filesystem_error& e) {
            std::cerr << "Failed to create log directory: " << e.what() << std::endl;
            return;
        }
    }

    logFile.open(filepath, std::ios::app);
    if (!logFile.is_open()) {
        std::cerr << "Failed to open log file: " << filepath << std::endl;
        fileEnabled = false;
    } else {
        fileEnabled = true;
    }
}

void Logger::enableConsoleOutput(bool enable) {
    std::lock_guard<std::mutex> lock(logMutex);
    consoleEnabled = enable;
}

void Logger::enableFileOutput(bool enable) {
    std::lock_guard<std::mutex> lock(logMutex);
    fileEnabled = enable;
}

void Logger::log(LogLevel level, const std::string& message,
                const std::string& file, int line) {
    if (level < currentLevel) {
        return;
    }

    std::ostringstream oss;
    oss << "[" << getCurrentTimestamp() << "] "
        << "[" << levelToString(level) << "] ";

    if (!file.empty() && line > 0) {
        fs::path filepath(file);
        oss << "[" << filepath.filename().string() << ":" << line << "] ";
    }

    oss << message;

    writeLog(oss.str());
}

void Logger::debug(const std::string& message, const std::string& file, int line) {
    log(LogLevel::DEBUG, message, file, line);
}

void Logger::info(const std::string& message, const std::string& file, int line) {
    log(LogLevel::INFO, message, file, line);
}

void Logger::warning(const std::string& message, const std::string& file, int line) {
    log(LogLevel::WARNING, message, file, line);
}

void Logger::error(const std::string& message, const std::string& file, int line) {
    log(LogLevel::ERROR, message, file, line);
}

void Logger::fatal(const std::string& message, const std::string& file, int line) {
    log(LogLevel::FATAL, message, file, line);
}

std::string Logger::levelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::DEBUG:   return "DEBUG";
        case LogLevel::INFO:    return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR:   return "ERROR";
        case LogLevel::FATAL:   return "FATAL";
        default:                return "UNKNOWN";
    }
}

std::string Logger::getCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S")
        << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

void Logger::writeLog(const std::string& formattedMessage) {
    std::lock_guard<std::mutex> lock(logMutex);

    if (consoleEnabled) {
        std::cout << formattedMessage << std::endl;
    }

    if (fileEnabled && logFile.is_open()) {
        logFile << formattedMessage << std::endl;
        logFile.flush();
    }
}