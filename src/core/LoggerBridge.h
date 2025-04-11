#ifndef LOGGERBRIDGE_H
#define LOGGERBRIDGE_H

#include <string>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <fstream>
#include <iostream>
#include <chrono>
#include <vector>
#include <unordered_map>

namespace Core {

/**
 * @brief 
 */
enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3,
    CRITICAL = 4
};

/**
 * @brief 日志消息结构
 */
struct LogMessage {
    LogLevel level;
    std::string message;
    std::chrono::system_clock::time_point timestamp;
    std::string category;
    std::string operation;
    bool isTroubleshooting;
};

/**
 * @brief 日志工具类，提供格式化和时间戳等功能
 */
class LogUtils {
public:
    /**
     * @brief 获取当前时间戳字符串
     * @return 格式化的时间戳字符串
     */
    static std::string getTimestamp();
    
    /**
     * @brief 获取日志级别的字符串表示
     * @param level 日志级别
     * @return 日志级别的字符串表示
     */
    static std::string getLevelString(LogLevel level);
    
    /**
     * @brief 格式化日志消息
     * @param format 格式字符串
     * @param args 参数列表
     * @return 格式化后的字符串
     */
    template<typename... Args>
    static std::string format(const std::string& format, Args&&... args);
    
    /**
     * @brief 格式化基本日志消息
     * @param level 日志级别
     * @param message 日志消息
     * @return 格式化后的日志字符串
     */
    static std::string formatLogMessage(LogLevel level, const std::string& message);
    
    /**
     * @brief 格式化故障排查日志消息
     * @param category 分类
     * @param operation 操作
     * @param message 日志消息
     * @return 格式化后的日志字符串
     */
    static std::string formatTroubleshootingMessage(
        const std::string& category, 
        const std::string& operation, 
        const std::string& message);
};

/**
 * @brief 日志桥接类，替代Qt日志操作
 */
class LoggerBridge {
public:
    /**
     * @brief 获取单例实例
     * @return LoggerBridge单例引用
     */
    static LoggerBridge& getInstance();
    
    /**
     * @brief 析构函数
     */
    ~LoggerBridge();
    
    /**
     * @brief 初始化日志系统
     * @param logFilePath 日志文件路径
     * @param troubleshootingLogPath 故障排查日志文件路径
     * @param consoleOutput 是否输出到控制台
     * @param minLevel 最小日志级别
     * @return 初始化是否成功
     */
    bool initialize(
        const std::string& logFilePath,
        const std::string& troubleshootingLogPath,
        bool consoleOutput = true,
        LogLevel minLevel = LogLevel::INFO);
    
    /**
     * @brief 关闭日志系统
     */
    void shutdown();
    
    /**
     * @brief 设置最小日志级别
     * @param level 日志级别
     */
    void setMinLevel(LogLevel level);
    
    /**
     * @brief 设置是否输出到控制台
     * @param enable 是否启用
     */
    void setConsoleOutput(bool enable);
    
    /**
     * @brief 记录调试级别日志
     * @param message 日志消息
     */
    void debug(const std::string& message);
    
    /**
     * @brief 记录信息级别日志
     * @param message 日志消息
     */
    void info(const std::string& message);
    
    /**
     * @brief 记录警告级别日志
     * @param message 日志消息
     */
    void warning(const std::string& message);
    
    /**
     * @brief 记录错误级别日志
     * @param message 日志消息
     */
    void error(const std::string& message);
    
    /**
     * @brief 记录严重错误级别日志
     * @param message 日志消息
     */
    void critical(const std::string& message);
    
    /**
     * @brief 记录故障排查日志
     * @param category 分类
     * @param operation 操作
     * @param message 日志消息
     */
    void troubleshootingLog(
        const std::string& category,
        const std::string& operation,
        const std::string& message);
    
    /**
     * @brief 记录简化的故障排查日志
     * @param message 日志消息
     */
    void troubleshootingLogMessage(const std::string& message);
    
    /**
     * @brief 记录过滤器操作的故障排查日志
     * @param operation 操作
     * @param message 日志消息
     */
    void troubleshootingLogFilterOperation(
        const std::string& operation,
        const std::string& message);
    
    /**
     * @brief 设置日志回调函数
     * @param callback 回调函数
     */
    using LogCallback = std::function<void(LogLevel, const std::string&)>;
    void setLogCallback(LogCallback callback);
    
    /**
     * @brief 格式化调试级别日志
     * @param format 格式字符串
     * @param args 参数列表
     */
    template<typename... Args>
    void debugf(const std::string& format, Args&&... args) {
        debug(LogUtils::format(format, std::forward<Args>(args)...));
    }
    
    /**
     * @brief 格式化信息级别日志
     * @param format 格式字符串
     * @param args 参数列表
     */
    template<typename... Args>
    void infof(const std::string& format, Args&&... args) {
        info(LogUtils::format(format, std::forward<Args>(args)...));
    }
    
    /**
     * @brief 格式化警告级别日志
     * @param format 格式字符串
     * @param args 参数列表
     */
    template<typename... Args>
    void warningf(const std::string& format, Args&&... args) {
        warning(LogUtils::format(format, std::forward<Args>(args)...));
    }
    
    /**
     * @brief 格式化错误级别日志
     * @param format 格式字符串
     * @param args 参数列表
     */
    template<typename... Args>
    void errorf(const std::string& format, Args&&... args) {
        error(LogUtils::format(format, std::forward<Args>(args)...));
    }
    
    /**
     * @brief 格式化严重错误级别日志
     * @param format 格式字符串
     * @param args 参数列表
     */
    template<typename... Args>
    void criticalf(const std::string& format, Args&&... args) {
        critical(LogUtils::format(format, std::forward<Args>(args)...));
    }

private:
    /**
     * @brief 私有构造函数（单例模式）
     */
    LoggerBridge();
    
    /**
     * @brief 禁止拷贝构造（单例模式）
     */
    LoggerBridge(const LoggerBridge&) = delete;
    
    /**
     * @brief 禁止赋值操作（单例模式）
     */
    LoggerBridge& operator=(const LoggerBridge&) = delete;
    
    /**
     * @brief 记录日志消息
     * @param level 日志级别
     * @param message 日志消息
     */
    void log(LogLevel level, const std::string& message);
    
    /**
     * @brief 处理日志消息
     * @param message 日志消息
     */
    void processLogMessage(const LogMessage& message);
    
    /**
     * @brief 日志处理线程函数
     */
    void logProcessingThread();
    
    /**
     * @brief 写入日志到文件
     * @param message 日志消息
     */
    void writeToFile(const LogMessage& message);
    
    /**
     * @brief 写入日志到控制台
     * @param message 日志消息
     */
    void writeToConsole(const LogMessage& message);
    
    /**
     * @brief 调用日志回调函数
     * @param message 日志消息
     */
    void callLogCallback(const LogMessage& message);
    
    /**
     * @brief 检查并轮转日志文件
     */
    void checkAndRotateLogFile();
    
    // 成员变量
    std::atomic<bool> m_initialized{false};
    std::atomic<bool> m_running{false};
    std::atomic<LogLevel> m_minLevel{LogLevel::INFO};
    std::atomic<bool> m_consoleOutput{true};
    
    std::string m_logFilePath;
    std::string m_troubleshootingLogPath;
    std::ofstream m_logFile;
    std::ofstream m_troubleshootingLogFile;
    
    std::mutex m_queueMutex;
    std::condition_variable m_queueCondition;
    std::queue<LogMessage> m_messageQueue;
    
    std::thread m_processingThread;
    
    std::mutex m_callbackMutex;
    LogCallback m_logCallback;
    
    static constexpr size_t MAX_QUEUE_SIZE = 1000;
    static constexpr size_t MAX_LOG_FILE_SIZE = 10 * 1024 * 1024; // 10MB
};

// 模板方法实现
template<typename... Args>
std::string LogUtils::format(const std::string& format, Args&&... args) {
    // 简单实现，实际项目中可以使用fmt库或C++20的std::format
    // 这里仅作为示例，实际实现需要更复杂的格式化逻辑
    std::string result = format;
    // 实现格式化逻辑...
    return result;
}

} // namespace Core

#endif // LOGGERBRIDGE_H 