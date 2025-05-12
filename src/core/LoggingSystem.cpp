#include "LoggingSystem.h"
#include <format>
#include <filesystem>

namespace Core {

//
// 辅助函数实现
//

std::string getTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    
    return ss.str();
}

std::string getLevelString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:    return "DEBUG";
        case LogLevel::INFO:     return "INFO";
        case LogLevel::WARNING:  return "WARNING";
        case LogLevel::ERROR:    return "ERROR";
        case LogLevel::CRITICAL: return "CRITICAL";
        default:                 return "UNKNOWN";
    }
}

std::string formatLogMessage(const LogMessage& message) {
    std::stringstream ss;
    ss << "[" << getTimestamp() << "] ";
    
    if (message.isTroubleshooting) {
        ss << "[" << message.category << "][" << message.operation << "] ";
    } else {
        ss << "[" << getLevelString(message.level) << "] ";
    }
    
    ss << message.message;
    
    // 添加文件、行号信息
    if (message.context.file && *message.context.file) {
        ss << " [" << message.context.file;
        if (message.context.line > 0) {
            ss << ":" << message.context.line;
        }
        ss << "]";
    }
    
    // 添加模块信息
    if (!message.context.module.empty()) {
        ss << " [" << message.context.module << "]";
    }
    
    return ss.str();
}

template<typename... Args>
std::string formatString(const std::string& format, Args&&... args) {
    try {
        return std::vformat(format, std::make_format_args(std::forward<Args>(args)...));
    } catch (const std::exception&) {
        // 如果格式化失败，返回原始字符串
        return format;
    }
}

//
// ConsoleSink 实现
//

void ConsoleSink::write(const LogMessage& message) {
    std::string formattedMessage = formatLogMessage(message);
    
    // 根据日志级别选择输出流并设置颜色（仅限终端支持颜色的环境）
    if (message.level >= LogLevel::ERROR) {
        std::cerr << formattedMessage << std::endl;
    } else {
        std::cout << formattedMessage << std::endl;
    }
}

//
// FileSink 实现
//

FileSink::FileSink(const std::string& filePath, size_t maxSize)
    : m_filePath(filePath), m_maxSize(maxSize) {
    
    // 创建目录（如果不存在）
    try {
        std::filesystem::path path(filePath);
        std::filesystem::create_directories(path.parent_path());
    } catch (const std::exception&) {
        // 创建目录失败，将在打开文件时处理错误
    }
    
    // 打开日志文件
    m_file.open(filePath, std::ios::app);
    if (!m_file) {
        std::cerr << "Failed to open log file: " << filePath << std::endl;
    }
}

FileSink::~FileSink() {
    if (m_file.is_open()) {
        m_file.close();
    }
}

void FileSink::write(const LogMessage& message) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // 检查并执行日志轮转
    checkRotation();
    
    // 格式化并写入日志消息
    if (m_file.is_open()) {
        std::string formattedMessage = formatLogMessage(message);
        m_file << formattedMessage << std::endl;
    }
}

void FileSink::flush() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_file.is_open()) {
        m_file.flush();
    }
}

void FileSink::checkRotation() {
    if (!m_file.is_open()) return;
    
    // 获取当前文件大小
    m_file.flush();
    size_t fileSize = 0;
    
    try {
        fileSize = std::filesystem::file_size(m_filePath);
    } catch (const std::exception&) {
        return; // 无法获取文件大小
    }
    
    // 如果文件大小超过限制，进行轮转
    if (fileSize > m_maxSize) {
        // 关闭当前文件
        m_file.close();
        
        // 生成轮转文件名（添加时间戳）
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::system_clock::to_time_t(now);
        std::string rotatedFilePath = m_filePath + "." + std::to_string(timestamp);
        
        // 重命名当前日志文件
        try {
            std::filesystem::rename(m_filePath, rotatedFilePath);
        } catch (const std::exception&) {
            // 重命名失败，尝试直接打开并清空原文件
        }
        
        // 重新打开日志文件
        m_file.open(m_filePath, std::ios::out | std::ios::trunc);
        if (!m_file) {
            std::cerr << "Failed to reopen log file after rotation: " << m_filePath << std::endl;
        }
    }
}

//
// CallbackSink 实现
//

CallbackSink::CallbackSink(Callback callback) : m_callback(callback) {}

void CallbackSink::write(const LogMessage& message) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_callback) {
        m_callback(message);
    }
}

//
// LogManager::StreamProxy 实现
//

LogManager::StreamProxy::StreamProxy(LogManager& manager, LogLevel level, const LogContext& context)
    : m_manager(manager), m_level(level), m_context(context) {}

LogManager::StreamProxy::~StreamProxy() {
    // 在析构时，将累积的消息发送到日志管理器
    m_manager.log(m_level, m_stream.str(), m_context);
}

//
// LogManager 实现
//

LogManager& LogManager::getInstance() {
    static LogManager instance;
    return instance;
}

LogManager::LogManager() : m_running(false), m_minLevel(LogLevel::INFO) {
    // 在构造函数中初始化但不启动线程
}

LogManager::~LogManager() {
    // 确保在析构时关闭日志系统
    shutdown();
}

void LogManager::initialize() {
    if (m_running.load()) return;
    
    // 启动处理线程
    m_running.store(true);
    m_processingThread = std::thread(&LogManager::processingThread, this);
}

void LogManager::shutdown() {
    if (!m_running.load()) return;
    
    // 停止处理线程
    m_running.store(false);
    
    // 通知线程退出
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_queueCondition.notify_all();
    }
    
    // 等待线程结束
    if (m_processingThread.joinable()) {
        m_processingThread.join();
    }
    
    // 处理剩余的日志消息
    while (!m_messageQueue.empty()) {
        LogMessage message = m_messageQueue.front();
        m_messageQueue.pop();
        // 直接处理消息，而不是放入队列
        std::unique_lock<std::mutex> lock(m_sinksMutex);
        for (auto& sink : m_sinks) {
            sink->write(message);
        }
    }
    
    // 刷新所有接收器
    std::unique_lock<std::mutex> lock(m_sinksMutex);
    for (auto& sink : m_sinks) {
        sink->flush();
    }
}

void LogManager::addSink(std::shared_ptr<ILogSink> sink) {
    if (!sink) return;
    
    std::lock_guard<std::mutex> lock(m_sinksMutex);
    m_sinks.push_back(sink);
}

void LogManager::removeSink(std::shared_ptr<ILogSink> sink) {
    if (!sink) return;
    
    std::lock_guard<std::mutex> lock(m_sinksMutex);
    auto it = std::find(m_sinks.begin(), m_sinks.end(), sink);
    if (it != m_sinks.end()) {
        m_sinks.erase(it);
    }
}

void LogManager::log(LogLevel level, const std::string& message, const LogContext& context) {
    // 检查是否应该记录此级别的日志
    if (!shouldLog(level, context.module)) return;
    
    // 创建日志消息
    LogMessage logMessage;
    logMessage.level = level;
    logMessage.message = message;
    logMessage.context = context;
    logMessage.timestamp = std::chrono::system_clock::now();
    logMessage.isTroubleshooting = false;
    
    // 添加到队列并处理
    logImpl(logMessage);
}

void LogManager::log(LogLevel level, const std::string& message, const char* file, int line, const char* function) {
    LogContext context(file, line, function);
    log(level, message, context);
}

void LogManager::debug(const std::string& message, const LogContext& context) {
    log(LogLevel::DEBUG, message, context);
}

void LogManager::info(const std::string& message, const LogContext& context) {
    log(LogLevel::INFO, message, context);
}

void LogManager::warning(const std::string& message, const LogContext& context) {
    log(LogLevel::WARNING, message, context);
}

void LogManager::error(const std::string& message, const LogContext& context) {
    log(LogLevel::ERROR, message, context);
}

void LogManager::critical(const std::string& message, const LogContext& context) {
    log(LogLevel::CRITICAL, message, context);
}

void LogManager::troubleshooting(const std::string& category, const std::string& operation, 
                               const std::string& message, const LogContext& context) {
    // 故障排查日志通常始终记录，不受日志级别限制
    
    // 创建日志消息
    LogMessage logMessage;
    logMessage.level = LogLevel::INFO; // 默认为 INFO 级别
    logMessage.message = message;
    logMessage.context = context;
    logMessage.timestamp = std::chrono::system_clock::now();
    logMessage.category = category;
    logMessage.operation = operation;
    logMessage.isTroubleshooting = true;
    
    // 添加到队列并处理
    logImpl(logMessage);
}

LogManager::StreamProxy LogManager::debugStream(const LogContext& context) {
    return StreamProxy(*this, LogLevel::DEBUG, context);
}

LogManager::StreamProxy LogManager::infoStream(const LogContext& context) {
    return StreamProxy(*this, LogLevel::INFO, context);
}

LogManager::StreamProxy LogManager::warningStream(const LogContext& context) {
    return StreamProxy(*this, LogLevel::WARNING, context);
}

LogManager::StreamProxy LogManager::errorStream(const LogContext& context) {
    return StreamProxy(*this, LogLevel::ERROR, context);
}

LogManager::StreamProxy LogManager::criticalStream(const LogContext& context) {
    return StreamProxy(*this, LogLevel::CRITICAL, context);
}

void LogManager::setMinLogLevel(LogLevel level) {
    m_minLevel.store(level);
}

LogLevel LogManager::getMinLogLevel() const {
    return m_minLevel.load();
}

void LogManager::setModuleLogLevel(const std::string& module, LogLevel level) {
    std::lock_guard<std::mutex> lock(m_modulesMutex);
    m_moduleLevels[module] = level;
}

LogLevel LogManager::getModuleLogLevel(const std::string& module) const {
    std::lock_guard<std::mutex> lock(m_modulesMutex);
    auto it = m_moduleLevels.find(module);
    if (it != m_moduleLevels.end()) {
        return it->second;
    }
    return m_minLevel.load();
}

void LogManager::logImpl(const LogMessage& message) {
    // 如果没有接收器，直接返回
    {
        std::lock_guard<std::mutex> lock(m_sinksMutex);
        if (m_sinks.empty()) return;
    }
    
    // 如果处理线程未运行，直接处理消息
    if (!m_running.load()) {
        std::unique_lock<std::mutex> lock(m_sinksMutex);
        for (auto& sink : m_sinks) {
            sink->write(message);
        }
        return;
    }
    
    // 添加到消息队列
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        
        // 如果队列已满，丢弃最旧的消息
        if (m_messageQueue.size() >= MAX_QUEUE_SIZE) {
            m_messageQueue.pop();
        }
        
        m_messageQueue.push(message);
    }
    
    // 通知处理线程
    m_queueCondition.notify_one();
}

void LogManager::processingThread() {
    while (m_running.load()) {
        LogMessage message;
        bool hasMessage = false;
        
        // 从队列中获取消息
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            
            // 等待消息或退出信号
            m_queueCondition.wait(lock, [this] {
                return !m_messageQueue.empty() || !m_running.load();
            });
            
            // 检查是否需要退出
            if (!m_running.load() && m_messageQueue.empty()) {
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
            std::unique_lock<std::mutex> lock(m_sinksMutex);
            for (auto& sink : m_sinks) {
                sink->write(message);
            }
        }
    }
}

bool LogManager::shouldLog(LogLevel level, const std::string& module) const {
    // 如果是模块特定日志，检查模块级别
    if (!module.empty()) {
        std::lock_guard<std::mutex> lock(m_modulesMutex);
        auto it = m_moduleLevels.find(module);
        if (it != m_moduleLevels.end()) {
            return level >= it->second;
        }
    }
    
    // 否则使用全局最小级别
    return level >= m_minLevel.load();
}

} // namespace Core