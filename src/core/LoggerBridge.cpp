#include "LoggerBridge.h"
#include <iomanip>
#include <sstream>
#include <ctime>
#include <filesystem>
#include <algorithm>

namespace Core {

// LogUtils 实现
std::string LogUtils::getTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    
    return ss.str();
}

std::string LogUtils::getLevelString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:    return "DEBUG";
        case LogLevel::INFO:     return "INFO";
        case LogLevel::WARNING:  return "WARNING";
        case LogLevel::ERROR:    return "ERROR";
        case LogLevel::CRITICAL: return "CRITICAL";
        default:                 return "UNKNOWN";
    }
}

std::string LogUtils::formatLogMessage(LogLevel level, const std::string& message) {
    std::stringstream ss;
    ss << "[" << getTimestamp() << "] [" << getLevelString(level) << "] " << message;
    return ss.str();
}

std::string LogUtils::formatTroubleshootingMessage(
    const std::string& category, 
    const std::string& operation, 
    const std::string& message) {
    
    std::stringstream ss;
    ss << "[" << getTimestamp() << "] [" << category << "][" << operation << "] " << message;
    return ss.str();
}

// LoggerBridge 实现
LoggerBridge& LoggerBridge::getInstance() {
    static LoggerBridge instance;
    return instance;
}

LoggerBridge::LoggerBridge() : m_initialized(false), m_running(false) {
    // 构造函数不做实际初始化，初始化在initialize方法中进行
}

LoggerBridge::~LoggerBridge() {
    shutdown();
}

bool LoggerBridge::initialize(
    const std::string& logFilePath,
    const std::string& troubleshootingLogPath,
    bool consoleOutput,
    LogLevel minLevel) {
    
    // 防止重复初始化
    if (m_initialized) {
        return true;
    }
    
    m_logFilePath = logFilePath;
    m_troubleshootingLogPath = troubleshootingLogPath;
    m_consoleOutput = consoleOutput;
    m_minLevel = minLevel;
    
    // 确保日志目录存在
    try {
        std::filesystem::path logDir = std::filesystem::path(logFilePath).parent_path();
        std::filesystem::path troubleshootingDir = std::filesystem::path(troubleshootingLogPath).parent_path();
        
        if (!logDir.empty() && !std::filesystem::exists(logDir)) {
            std::filesystem::create_directories(logDir);
        }
        
        if (!troubleshootingDir.empty() && !std::filesystem::exists(troubleshootingDir)) {
            std::filesystem::create_directories(troubleshootingDir);
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to create log directories: " << e.what() << std::endl;
        return false;
    }
    
    // 打开日志文件
    m_logFile.open(logFilePath, std::ios::trunc);
    if (!m_logFile) {
        std::cerr << "Failed to open log file: " << logFilePath << std::endl;
        return false;
    }
    
    m_troubleshootingLogFile.open(troubleshootingLogPath, std::ios::app);
    if (!m_troubleshootingLogFile) {
        std::cerr << "Failed to open troubleshooting log file: " << troubleshootingLogPath << std::endl;
        m_logFile.close();
        return false;
    }
    
    // 启动日志处理线程
    m_running = true;
    m_processingThread = std::thread(&LoggerBridge::logProcessingThread, this);
    
    m_initialized = true;
    
    // 记录初始化成功日志
    info("Logger initialized successfully");
    
    return true;
}

void LoggerBridge::shutdown() {
    if (!m_initialized) {
        return;
    }
    
    // 停止处理线程
    m_running = false;
    
    // 通知处理线程退出
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_queueCondition.notify_all();
    }
    
    // 等待处理线程结束
    if (m_processingThread.joinable()) {
        m_processingThread.join();
    }
    
    // 处理剩余的日志消息
    while (!m_messageQueue.empty()) {
        LogMessage message = m_messageQueue.front();
        m_messageQueue.pop();
        processLogMessage(message);
    }
    
    // 关闭日志文件
    if (m_logFile.is_open()) {
        m_logFile.close();
    }
    
    if (m_troubleshootingLogFile.is_open()) {
        m_troubleshootingLogFile.close();
    }
    
    m_initialized = false;
}

void LoggerBridge::setMinLevel(LogLevel level) {
    m_minLevel = level;
}

void LoggerBridge::setConsoleOutput(bool enable) {
    m_consoleOutput = enable;
}

void LoggerBridge::debug(const std::string& message) {
    log(LogLevel::DEBUG, message);
}

void LoggerBridge::info(const std::string& message) {
    log(LogLevel::INFO, message);
}

void LoggerBridge::warning(const std::string& message) {
    log(LogLevel::WARNING, message);
}

void LoggerBridge::error(const std::string& message) {
    log(LogLevel::ERROR, message);
}

void LoggerBridge::critical(const std::string& message) {
    log(LogLevel::CRITICAL, message);
}

void LoggerBridge::troubleshootingLog(
    const std::string& category,
    const std::string& operation,
    const std::string& message) {
    
    if (!m_initialized) {
        std::cerr << "Logger not initialized" << std::endl;
        return;
    }
    
    LogMessage logMessage;
    logMessage.level = LogLevel::INFO;
    logMessage.message = message;
    logMessage.timestamp = std::chrono::system_clock::now();
    logMessage.category = category;
    logMessage.operation = operation;
    logMessage.isTroubleshooting = true;
    
    // 添加到消息队列
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        
        // 队列已满，丢弃最旧的消息
        if (m_messageQueue.size() >= MAX_QUEUE_SIZE) {
            m_messageQueue.pop();
        }
        
        m_messageQueue.push(logMessage);
    }
    
    // 通知处理线程
    m_queueCondition.notify_one();
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
}

void LoggerBridge::log(LogLevel level, const std::string& message) {
    if (!m_initialized) {
        std::cerr << "Logger not initialized" << std::endl;
        return;
    }
    
    // 检查日志级别
    if (level < m_minLevel) {
        return;
    }
    
    LogMessage logMessage;
    logMessage.level = level;
    logMessage.message = message;
    logMessage.timestamp = std::chrono::system_clock::now();
    logMessage.isTroubleshooting = false;
    
    // 添加到消息队列
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        
        // 队列已满，丢弃最旧的消息
        if (m_messageQueue.size() >= MAX_QUEUE_SIZE) {
            m_messageQueue.pop();
        }
        
        m_messageQueue.push(logMessage);
    }
    
    // 通知处理线程
    m_queueCondition.notify_one();
}

void LoggerBridge::logProcessingThread() {
    while (m_running) {
        LogMessage message;
        bool hasMessage = false;
        
        // 从队列中获取消息
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            
            // 等待消息或退出信号
            m_queueCondition.wait(lock, [this] {
                return !m_messageQueue.empty() || !m_running;
            });
            
            // 检查是否需要退出
            if (!m_running && m_messageQueue.empty()) {
                break;
            }
            
            // 获取消息
            if (!m_messageQueue.empty()) {
                message = m_messageQueue.front();
                m_messageQueue.pop();
                hasMessage = true;
            }
        }
        
        // 处理消息
        if (hasMessage) {
            processLogMessage(message);
        }
    }
}

void LoggerBridge::processLogMessage(const LogMessage& message) {
    // 写入日志文件
    writeToFile(message);
    
    // 输出到控制台
    if (m_consoleOutput) {
        writeToConsole(message);
    }
    
    // 调用回调函数
    callLogCallback(message);
}

void LoggerBridge::writeToFile(const LogMessage& message) {
    // 检查并轮转日志文件
    checkAndRotateLogFile();
    
    // 格式化日志消息
    std::string formattedMessage;
    
    if (message.isTroubleshooting) {
        formattedMessage = LogUtils::formatTroubleshootingMessage(
            message.category, message.operation, message.message);
        
        // 写入故障排查日志文件
        if (m_troubleshootingLogFile.is_open()) {
            m_troubleshootingLogFile << formattedMessage << std::endl;
            m_troubleshootingLogFile.flush();
        }
    } else {
        formattedMessage = LogUtils::formatLogMessage(message.level, message.message);
        
        // 写入普通日志文件
        if (m_logFile.is_open()) {
            m_logFile << formattedMessage << std::endl;
            m_logFile.flush();
        }
    }
}

void LoggerBridge::writeToConsole(const LogMessage& message) {
    // 格式化日志消息
    std::string formattedMessage;
    
    if (message.isTroubleshooting) {
        formattedMessage = LogUtils::formatTroubleshootingMessage(
            message.category, message.operation, message.message);
    } else {
        formattedMessage = LogUtils::formatLogMessage(message.level, message.message);
    }
    
    // 根据日志级别选择输出流
    if (message.level >= LogLevel::ERROR) {
        std::cerr << formattedMessage << std::endl;
    } else {
        std::cout << formattedMessage << std::endl;
    }
}

void LoggerBridge::callLogCallback(const LogMessage& message) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    
    if (m_logCallback) {
        std::string formattedMessage;
        
        if (message.isTroubleshooting) {
            formattedMessage = LogUtils::formatTroubleshootingMessage(
                message.category, message.operation, message.message);
        } else {
            formattedMessage = LogUtils::formatLogMessage(message.level, message.message);
        }
        
        m_logCallback(message.level, formattedMessage);
    }
}

void LoggerBridge::checkAndRotateLogFile() {
    // 检查日志文件大小
    if (m_logFile.is_open()) {
        m_logFile.flush();
        
        // 获取文件大小
        std::streampos fileSize = 0;
        try {
            fileSize = std::filesystem::file_size(m_logFilePath);
        } catch (const std::exception& e) {
            std::cerr << "Failed to get log file size: " << e.what() << std::endl;
            return;
        }
        
        // 如果文件大小超过限制，进行轮转
        if (fileSize > MAX_LOG_FILE_SIZE) {
            // 关闭当前日志文件
            m_logFile.close();
            
            // 生成轮转文件名
            std::string rotatedFilePath = m_logFilePath + "." + 
                std::to_string(std::chrono::system_clock::to_time_t(
                    std::chrono::system_clock::now()));
            
            // 重命名当前日志文件
            try {
                std::filesystem::rename(m_logFilePath, rotatedFilePath);
            } catch (const std::exception& e) {
                std::cerr << "Failed to rotate log file: " << e.what() << std::endl;
            }
            
            // 重新打开日志文件
            m_logFile.open(m_logFilePath, std::ios::app);
            if (!m_logFile) {
                std::cerr << "Failed to reopen log file after rotation: " << m_logFilePath << std::endl;
            }
        }
    }
    
    // 同样检查故障排查日志文件
    if (m_troubleshootingLogFile.is_open()) {
        m_troubleshootingLogFile.flush();
        
        // 获取文件大小
        std::streampos fileSize = 0;
        try {
            fileSize = std::filesystem::file_size(m_troubleshootingLogPath);
        } catch (const std::exception& e) {
            std::cerr << "Failed to get troubleshooting log file size: " << e.what() << std::endl;
            return;
        }
        
        // 如果文件大小超过限制，进行轮转
        if (fileSize > MAX_LOG_FILE_SIZE) {
            // 关闭当前日志文件
            m_troubleshootingLogFile.close();
            
            // 生成轮转文件名
            std::string rotatedFilePath = m_troubleshootingLogPath + "." + 
                std::to_string(std::chrono::system_clock::to_time_t(
                    std::chrono::system_clock::now()));
            
            // 重命名当前日志文件
            try {
                std::filesystem::rename(m_troubleshootingLogPath, rotatedFilePath);
            } catch (const std::exception& e) {
                std::cerr << "Failed to rotate troubleshooting log file: " << e.what() << std::endl;
            }
            
            // 重新打开日志文件
            m_troubleshootingLogFile.open(m_troubleshootingLogPath, std::ios::app);
            if (!m_troubleshootingLogFile) {
                std::cerr << "Failed to reopen troubleshooting log file after rotation: " 
                          << m_troubleshootingLogPath << std::endl;
            }
        }
    }
}

} // namespace Core 