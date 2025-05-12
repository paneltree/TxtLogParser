#include "LoggerBridge.h"
#include "LoggingSystem.h" // Include the new unified logging system
#include <iomanip>
#include <sstream>
#include <ctime>
#include <filesystem>
#include <algorithm>

namespace Core {

// LogUtils 实现
std::string LogUtils::getTimestamp() {
    return Core::getTimestamp(); // 直接使用统一日志系统的函数
}

std::string LogUtils::getLevelString(LogLevel level) {
    return Core::getLevelString(static_cast<Core::LogLevel>(level)); // 转换并使用统一日志系统的函数
}

std::string LogUtils::formatLogMessage(LogLevel level, const std::string& message) {
    // 创建一个临时的LogMessage对象，使用统一日志系统的格式化功能
    Core::LogMessage logMessage;
    logMessage.level = static_cast<Core::LogLevel>(level);
    logMessage.message = message;
    logMessage.timestamp = std::chrono::system_clock::now();
    logMessage.isTroubleshooting = false;
    
    return Core::formatLogMessage(logMessage);
}

std::string LogUtils::formatTroubleshootingMessage(
    const std::string& category, 
    const std::string& operation, 
    const std::string& message) {
    
    // 创建一个临时的LogMessage对象，使用统一日志系统的格式化功能
    Core::LogMessage logMessage;
    logMessage.level = Core::LogLevel::INFO;
    logMessage.message = message;
    logMessage.timestamp = std::chrono::system_clock::now();
    logMessage.category = category;
    logMessage.operation = operation;
    logMessage.isTroubleshooting = true;
    
    return Core::formatLogMessage(logMessage);
}

// LoggerBridge 实现
LoggerBridge& LoggerBridge::getInstance() {
    static LoggerBridge instance;
    return instance;
}

LoggerBridge::LoggerBridge() : m_initialized(false), m_running(false) {
    // 构造函数
}

LoggerBridge::~LoggerBridge() {
    shutdown();
}

bool LoggerBridge::initialize(
    const std::string& logFilePath,
    const std::string& troubleshootingLogPath,
    bool consoleOutput,
    LogLevel minLevel) {
    
    if (m_initialized) {
        return true; // 已经初始化
    }
    
    // 存储参数
    m_logFilePath = logFilePath;
    m_troubleshootingLogPath = troubleshootingLogPath;
    m_consoleOutput = consoleOutput;
    m_minLevel = minLevel;
    
    // 初始化统一日志系统
    auto& logManager = Core::LogManager::getInstance();
    logManager.initialize();
    logManager.setMinLogLevel(static_cast<Core::LogLevel>(minLevel));
    
    // 添加控制台接收器
    if (consoleOutput) {
        logManager.addSink(std::make_shared<Core::ConsoleSink>());
    }
    
    // 添加文件接收器
    try {
        // 为应用程序日志添加文件接收器
        logManager.addSink(std::make_shared<Core::FileSink>(logFilePath, MAX_LOG_FILE_SIZE));
        
        // 为故障排查日志添加专用文件接收器
        logManager.addSink(std::make_shared<Core::FileSink>(troubleshootingLogPath, MAX_LOG_FILE_SIZE));
        
        m_initialized = true;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize logging system: " << e.what() << std::endl;
        return false;
    }
}

void LoggerBridge::shutdown() {
    if (!m_initialized) {
        return;
    }
    
    // 关闭统一日志系统
    Core::LogManager::getInstance().shutdown();
    
    m_initialized = false;
}

void LoggerBridge::setMinLevel(LogLevel level) {
    m_minLevel = level;
    Core::LogManager::getInstance().setMinLogLevel(static_cast<Core::LogLevel>(level));
}

void LoggerBridge::setConsoleOutput(bool enable) {
    m_consoleOutput = enable;
    
    // 注意：这个方法在新的日志系统中需要重新添加或移除控制台接收器
    // 此处简化处理，仅更新标志
}

void LoggerBridge::debug(const std::string& message) {
    Core::LogManager::getInstance().debug(message);
}

void LoggerBridge::info(const std::string& message) {
    Core::LogManager::getInstance().info(message);
}

void LoggerBridge::warning(const std::string& message) {
    Core::LogManager::getInstance().warning(message);
}

void LoggerBridge::error(const std::string& message) {
    Core::LogManager::getInstance().error(message);
}

void LoggerBridge::critical(const std::string& message) {
    Core::LogManager::getInstance().critical(message);
}

void LoggerBridge::troubleshootingLog(
    const std::string& category,
    const std::string& operation,
    const std::string& message) {
    
    Core::LogManager::getInstance().troubleshooting(category, operation, message);
}

void LoggerBridge::troubleshootingLogMessage(const std::string& message) {
    troubleshootingLog("General", "Message", message);
}

void LoggerBridge::troubleshootingLogFilterOperation(
    const std::string& operation,
    const std::string& message) {
    
    troubleshootingLog("Filter", operation, message);
}

void LoggerBridge::setLogCallback(LogCallback callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_logCallback = callback;
    
    // 如果提供了回调，则添加到统一日志系统
    if (callback) {
        // 创建一个适配器将新的LogMessage转换为旧的格式
        auto callbackAdapter = [this](const Core::LogMessage& newMessage) {
            if (m_logCallback) {
                // 转换消息格式并调用旧的回调
                m_logCallback(
                    static_cast<LogLevel>(newMessage.level),
                    newMessage.message
                );
            }
        };
        
        // 添加回调接收器
        Core::LogManager::getInstance().addSink(
            std::make_shared<Core::CallbackSink>(callbackAdapter));
    }
}

void LoggerBridge::log(LogLevel level, const std::string& message) {
    Core::LogManager::getInstance().log(
        static_cast<Core::LogLevel>(level), 
        message
    );
}

// 以下方法在新的架构中不再需要，但为保持向后兼容性，保留空实现

void LoggerBridge::processLogMessage(const LogMessage& message) {
    // 新的日志系统会自动处理所有消息
}

void LoggerBridge::logProcessingThread() {
    // 新的日志系统有自己的处理线程
}

void LoggerBridge::writeToFile(const LogMessage& message) {
    // 新的日志系统会自动处理文件写入
}

void LoggerBridge::writeToConsole(const LogMessage& message) {
    // 新的日志系统会自动处理控制台输出
}

void LoggerBridge::callLogCallback(const LogMessage& message) {
    // 新的日志系统会自动处理回调
}

void LoggerBridge::checkAndRotateLogFile() {
    // 新的日志系统会自动处理日志轮转
}

// 实现LogUtils::format函数的模板特化(在头文件中声明但需要在cpp中实现)
template<>
std::string LogUtils::format(const std::string& format) {
    return format;
}

// 特化一些常见类型以避免编译错误
template std::string LogUtils::format<int>(const std::string&, int&&);
template std::string LogUtils::format<const char*>(const std::string&, const char*&&);
template std::string LogUtils::format<std::string>(const std::string&, std::string&&);

} // namespace Core