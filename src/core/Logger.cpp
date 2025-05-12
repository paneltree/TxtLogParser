#include "Logger.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <iostream>

namespace Core {

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger() {
    // Constructor
}

Logger::~Logger() {
    closeLogFile();
}

void Logger::debug(const std::string& message) {
    log(LogLevel::DEBUG, message);
}

void Logger::info(const std::string& message) {
    log(LogLevel::INFO, message);
}

void Logger::warning(const std::string& message) {
    log(LogLevel::WARNING, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::ERROR, message);
}

void Logger::critical(const std::string& message) {
    log(LogLevel::CRITICAL, message);
}

bool Logger::setLogFile(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(logMutex);
    
    // Close existing log file if open
    if (logFile.is_open()) {
        logFile.close();
    }
    
    // Open new log file
    logFile.open(filePath, std::ios::out | std::ios::app);
    if (!logFile.is_open()) {
        std::cerr << "Failed to open log file: " << filePath << std::endl;
        return false;
    }
    
    return true;
}

void Logger::closeLogFile() {
    std::lock_guard<std::mutex> lock(logMutex);
    if (logFile.is_open()) {
        logFile.close();
    }
}

void Logger::log(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(logMutex);
    
    std::string timestamp = getTimestamp();
    std::string levelStr = getLevelString(level);
    std::string formattedMessage = timestamp + " [" + levelStr + "] " + message;
    
    // Write to log file if open
    if (logFile.is_open()) {
        logFile << formattedMessage << std::endl;
        logFile.flush();
    }
    
    // Write to console
    std::cout << formattedMessage << std::endl;
    
    // Call callback if set
    if (logCallback) {
        logCallback(level, message);
    }
}

std::string Logger::getTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    
    return ss.str();
}

std::string Logger::getLevelString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:
            return "DEBUG";
        case LogLevel::INFO:
            return "INFO";
        case LogLevel::WARNING:
            return "WARNING";
        case LogLevel::ERROR:
            return "ERROR";
        case LogLevel::CRITICAL:
            return "CRITICAL";
        default:
            return "UNKNOWN";
    }
}

} // namespace Core 