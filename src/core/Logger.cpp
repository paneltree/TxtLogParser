#include "Logger.h"
#include "LoggingSystem.h"  // Include our new unified logging system
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
    
    // Close any open file
    closeLogFile();
    
    try {
        // Add a file sink to the LogManager
        LogManager::getInstance().addSink(std::make_shared<FileSink>(filePath));
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to set log file: " << e.what() << std::endl;
        return false;
    }
}

void Logger::closeLogFile() {
    // With our new design, we don't need to explicitly close files
    // as the FileSink's destructor will handle that
}

void Logger::log(LogLevel level, const std::string& message) {
    // Convert Logger::LogLevel to Core::LogLevel and delegate to LogManager
    Core::LogLevel coreLevel;
    
    switch (level) {
        case LogLevel::DEBUG:    coreLevel = Core::LogLevel::DEBUG; break;
        case LogLevel::INFO:     coreLevel = Core::LogLevel::INFO; break;
        case LogLevel::WARNING:  coreLevel = Core::LogLevel::WARNING; break;
        case LogLevel::ERROR:    coreLevel = Core::LogLevel::ERROR; break;
        case LogLevel::CRITICAL: coreLevel = Core::LogLevel::CRITICAL; break;
    }
    
    // Delegate to LogManager
    LogManager::getInstance().log(coreLevel, message);
    
    // Call the callback if set
    if (logCallback) {
        logCallback(level, message);
    }
}

std::string Logger::getTimestamp() {
    // Delegate to LoggingSystem's implementation
    return Core::getTimestamp();
}

std::string Logger::getLevelString(LogLevel level) {
    // Convert and delegate
    Core::LogLevel coreLevel;
    
    switch (level) {
        case LogLevel::DEBUG:    coreLevel = Core::LogLevel::DEBUG; break;
        case LogLevel::INFO:     coreLevel = Core::LogLevel::INFO; break;
        case LogLevel::WARNING:  coreLevel = Core::LogLevel::WARNING; break;
        case LogLevel::ERROR:    coreLevel = Core::LogLevel::ERROR; break;
        case LogLevel::CRITICAL: coreLevel = Core::LogLevel::CRITICAL; break;
    }
    
    return Core::getLevelString(coreLevel);
}

} // namespace Core