#ifndef CORE_LOGGINGSYSTEM_H
#define CORE_LOGGINGSYSTEM_H

#include <string>
#include <memory>
#include <vector>
#include <mutex>
#include <queue>
#include <thread>
#include <atomic>
#include <sstream>
#include <functional>
#include <condition_variable>
#include <unordered_map>
#include <fstream>
#include <iostream>
#include <chrono>
#include <iomanip>

namespace Core {

/**
 * @brief 统一的日志级别枚举
 */
enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3,
    CRITICAL = 4
};

/**
 * @brief 日志上下文信息结构体
 */
struct LogContext {
    const char* file = "";
    int line = 0;
    const char* function = "";
    std::string module = "";
    
    LogContext(const char* file = "", int line = 0, const char* function = "", const std::string& module = "")
        : file(file), line(line), function(function), module(module) {}
};

/**
 * @brief 日志消息结构体
 */
struct LogMessage {
    LogLevel level;
    std::string message;
    LogContext context;
    std::chrono::system_clock::time_point timestamp;
    
    // 故障排查日志专用字段
    std::string category;
    std::string operation;
    bool isTroubleshooting;
    
    LogMessage()
        : level(LogLevel::INFO), isTroubleshooting(false),
          timestamp(std::chrono::system_clock::now()) {}
};

/**
 * @brief 日志接收器抽象接口
 */
class ILogSink {
public:
    virtual ~ILogSink() = default;
    virtual void write(const LogMessage& message) = 0;
    virtual void flush() = 0;
};

/**
 * @brief 控制台日志接收器
 */
class ConsoleSink : public ILogSink {
public:
    void write(const LogMessage& message) override;
    void flush() override {}
};

/**
 * @brief 文件日志接收器（支持日志轮转）
 */
class FileSink : public ILogSink {
public:
    FileSink(const std::string& filePath, size_t maxSize = 10 * 1024 * 1024);
    ~FileSink();
    
    void write(const LogMessage& message) override;
    void flush() override;
    
private:
    void checkRotation();
    std::string m_filePath;
    std::ofstream m_file;
    size_t m_maxSize;
    std::mutex m_mutex;
};

/**
 * @brief 回调函数日志接收器（用于UI集成）
 */
class CallbackSink : public ILogSink {
public:
    using Callback = std::function<void(const LogMessage&)>;
    
    CallbackSink(Callback callback);
    void write(const LogMessage& message) override;
    void flush() override {}
    
private:
    Callback m_callback;
    std::mutex m_mutex;
};

/**
 * @brief 中央日志管理器（单例）
 */
class LogManager {
public:
    static LogManager& getInstance();
    
    // 添加和移除日志接收器
    void addSink(std::shared_ptr<ILogSink> sink);
    void removeSink(std::shared_ptr<ILogSink> sink);
    
    // 核心日志方法
    void log(LogLevel level, const std::string& message, const LogContext& context = LogContext());
    void log(LogLevel level, const std::string& message, const char* file, int line, const char* function = "");
    
    // 便捷日志方法
    void debug(const std::string& message, const LogContext& context = LogContext());
    void info(const std::string& message, const LogContext& context = LogContext());
    void warning(const std::string& message, const LogContext& context = LogContext());
    void error(const std::string& message, const LogContext& context = LogContext());
    void critical(const std::string& message, const LogContext& context = LogContext());
    
    // 故障排查日志
    void troubleshooting(const std::string& category, const std::string& operation, 
                       const std::string& message, const LogContext& context = LogContext());
    
    // 流式接口代理类
    class StreamProxy {
    public:
        StreamProxy(LogManager& manager, LogLevel level, const LogContext& context);
        ~StreamProxy();
        
        template<typename T>
        StreamProxy& operator<<(const T& value) {
            m_stream << value;
            return *this;
        }
        
    private:
        LogManager& m_manager;
        LogLevel m_level;
        LogContext m_context;
        std::ostringstream m_stream;
    };
    
    // 流式接口方法
    StreamProxy debugStream(const LogContext& context = LogContext());
    StreamProxy infoStream(const LogContext& context = LogContext());
    StreamProxy warningStream(const LogContext& context = LogContext());
    StreamProxy errorStream(const LogContext& context = LogContext());
    StreamProxy criticalStream(const LogContext& context = LogContext());
    
    // 格式化日志方法
    template<typename... Args>
    void debugf(const std::string& format, Args&&... args);
    
    template<typename... Args>
    void infof(const std::string& format, Args&&... args);
    
    template<typename... Args>
    void warningf(const std::string& format, Args&&... args);
    
    template<typename... Args>
    void errorf(const std::string& format, Args&&... args);
    
    template<typename... Args>
    void criticalf(const std::string& format, Args&&... args);
    
    // 配置方法
    void setMinLogLevel(LogLevel level);
    LogLevel getMinLogLevel() const;
    void setModuleLogLevel(const std::string& module, LogLevel level);
    LogLevel getModuleLogLevel(const std::string& module) const;
    
    // 初始化和关闭
    void initialize();
    void shutdown();
    
private:
    LogManager();
    ~LogManager();
    LogManager(const LogManager&) = delete;
    LogManager& operator=(const LogManager&) = delete;
    
    void logImpl(const LogMessage& message);
    void processingThread();
    bool shouldLog(LogLevel level, const std::string& module) const;
    
    std::vector<std::shared_ptr<ILogSink>> m_sinks;
    std::mutex m_sinksMutex;
    
    std::queue<LogMessage> m_messageQueue;
    std::mutex m_queueMutex;
    std::condition_variable m_queueCondition;
    
    std::thread m_processingThread;
    std::atomic<bool> m_running;
    std::atomic<LogLevel> m_minLevel;
    
    std::unordered_map<std::string, LogLevel> m_moduleLevels;
    std::mutex m_modulesMutex;
    
    static constexpr size_t MAX_QUEUE_SIZE = 1000;
};

// 实用函数
std::string getTimestamp();
std::string getLevelString(LogLevel level);
std::string formatLogMessage(const LogMessage& message);

// 字符串格式化函数
template<typename... Args>
std::string formatString(const std::string& format, Args&&... args);

} // namespace Core

// 模板方法实现
namespace Core {

template<typename... Args>
void LogManager::debugf(const std::string& format, Args&&... args) {
    debug(formatString(format, std::forward<Args>(args)...));
}

template<typename... Args>
void LogManager::infof(const std::string& format, Args&&... args) {
    info(formatString(format, std::forward<Args>(args)...));
}

template<typename... Args>
void LogManager::warningf(const std::string& format, Args&&... args) {
    warning(formatString(format, std::forward<Args>(args)...));
}

template<typename... Args>
void LogManager::errorf(const std::string& format, Args&&... args) {
    error(formatString(format, std::forward<Args>(args)...));
}

template<typename... Args>
void LogManager::criticalf(const std::string& format, Args&&... args) {
    critical(formatString(format, std::forward<Args>(args)...));
}

}

// 用户友好的宏
#define LOG_DEBUG(message) \
    Core::LogManager::getInstance().debug(message, {__FILE__, __LINE__, __FUNCTION__})

#define LOG_INFO(message) \
    Core::LogManager::getInstance().info(message, {__FILE__, __LINE__, __FUNCTION__})

#define LOG_WARNING(message) \
    Core::LogManager::getInstance().warning(message, {__FILE__, __LINE__, __FUNCTION__})

#define LOG_ERROR(message) \
    Core::LogManager::getInstance().error(message, {__FILE__, __LINE__, __FUNCTION__})

#define LOG_CRITICAL(message) \
    Core::LogManager::getInstance().critical(message, {__FILE__, __LINE__, __FUNCTION__})

#define LOG_TROUBLESHOOTING(category, operation, message) \
    Core::LogManager::getInstance().troubleshooting(category, operation, message, {__FILE__, __LINE__, __FUNCTION__})

// 流式日志宏
#define LOG_DEBUG_STREAM() \
    Core::LogManager::getInstance().debugStream({__FILE__, __LINE__, __FUNCTION__})

#define LOG_INFO_STREAM() \
    Core::LogManager::getInstance().infoStream({__FILE__, __LINE__, __FUNCTION__})

#define LOG_WARNING_STREAM() \
    Core::LogManager::getInstance().warningStream({__FILE__, __LINE__, __FUNCTION__})

#define LOG_ERROR_STREAM() \
    Core::LogManager::getInstance().errorStream({__FILE__, __LINE__, __FUNCTION__})

#define LOG_CRITICAL_STREAM() \
    Core::LogManager::getInstance().criticalStream({__FILE__, __LINE__, __FUNCTION__})

// 模块特定日志宏
#define MODULE_LOG_DEBUG(module, message) \
    Core::LogManager::getInstance().debug(message, {__FILE__, __LINE__, __FUNCTION__, module})

#define MODULE_LOG_INFO(module, message) \
    Core::LogManager::getInstance().info(message, {__FILE__, __LINE__, __FUNCTION__, module})

#define MODULE_LOG_WARNING(module, message) \
    Core::LogManager::getInstance().warning(message, {__FILE__, __LINE__, __FUNCTION__, module})

#define MODULE_LOG_ERROR(module, message) \
    Core::LogManager::getInstance().error(message, {__FILE__, __LINE__, __FUNCTION__, module})

// 向后兼容宏（为现有代码提供兼容性）
#ifdef ENABLE_LEGACY_LOGGING
    #define DEBUG_STREAM() LOG_DEBUG_STREAM()
    #define INFO_STREAM() LOG_INFO_STREAM()
    #define WARNING_STREAM() LOG_WARNING_STREAM()
    #define ERROR_STREAM() LOG_ERROR_STREAM()
    #define CRITICAL_STREAM() LOG_CRITICAL_STREAM()
    
    #define DEBUG_LOG(message) LOG_DEBUG(message)
    #define INFO_LOG(message) LOG_INFO(message)
    #define WARNING_LOG(message) LOG_WARNING(message)
    #define ERROR_LOG(message) LOG_ERROR(message)
    #define CRITICAL_LOG(message) LOG_CRITICAL(message)
    
    #define MODULE_DEBUG_LOG(module, message) MODULE_LOG_DEBUG(module, message)
#endif

// Qt兼容宏
#define qDebug() LOG_DEBUG_STREAM()
#define qInfo() LOG_INFO_STREAM()
#define qWarning() LOG_WARNING_STREAM()
#define qCritical() LOG_ERROR_STREAM()
#define qFatal(message) LOG_CRITICAL(message)

#endif // CORE_LOGGINGSYSTEM_H