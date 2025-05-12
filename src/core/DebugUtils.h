#ifndef CORE_DEBUGUTILS_H
#define CORE_DEBUGUTILS_H

#include <string>
#include <sstream>
#include <cassert>
#include <functional>
#include <unordered_map>
#include <memory>
#include <exception>
#include "LoggingSystem.h"  // Include our new unified logging system
#include "LoggerBridge.h"   // Keep for backward compatibility

namespace Core {

/**
 * @brief 调试流类，支持流式语法
 * 
 * 此类用于替代Qt的QDebug流式语法，提供类似的使用体验
 * 该类现在是LogManager::StreamProxy的适配器
 */
class DebugStream {
public:
    /**
     * @brief 构造函数
     * @param level 日志级别
     * @param file 文件名
     * @param line 行号
     */
    DebugStream(LogLevel level, const char* file, int line)
        : m_level(level), m_file(file), m_line(line) {}
    
    /**
     * @brief 析构函数，输出累积的日志消息
     */
    ~DebugStream() {
        // 创建上下文
        LogContext context(m_file, m_line);
        
        // 获取累积的消息
        std::string message = m_stream.str();
        
        // 根据日志级别，将消息发送到统一日志系统
        switch (m_level) {
            case LogLevel::DEBUG:
                LogManager::getInstance().debug(message, context);
                break;
            case LogLevel::INFO:
                LogManager::getInstance().info(message, context);
                break;
            case LogLevel::WARNING:
                LogManager::getInstance().warning(message, context);
                break;
            case LogLevel::ERROR:
                LogManager::getInstance().error(message, context);
                break;
            case LogLevel::CRITICAL:
                LogManager::getInstance().critical(message, context);
                break;
        }
    }
    
    /**
     * @brief 通用流操作符
     * @param value 要输出的值
     * @return 流对象引用
     */
    template<typename T>
    DebugStream& operator<<(const T& value) {
        m_stream << value;
        return *this;
    }
    
    /**
     * @brief 特化版本，处理字符串
     */
    DebugStream& operator<<(const std::string& value) {
        m_stream << value;
        return *this;
    }
    
    /**
     * @brief 特化版本，处理C风格字符串
     */
    DebugStream& operator<<(const char* value) {
        m_stream << value;
        return *this;
    }
    
private:
    std::stringstream m_stream;  ///< 累积日志消息的流
    LogLevel m_level;            ///< 日志级别
    const char* m_file;          ///< 文件名
    int m_line;                  ///< 行号
};

/**
 * @brief 日志配置类，管理模块级日志配置
 * 
 * 该类现在是LogManager的适配器
 */
class LogConfig {
public:
    /**
     * @brief 设置模块日志级别
     * @param module 模块名称
     * @param level 日志级别
     */
    static void setModuleLogLevel(const std::string& module, LogLevel level) {
        // 将旧的日志级别转换为新的日志级别
        Core::LogLevel newLevel = static_cast<Core::LogLevel>(level);
        
        // 在统一日志系统中设置模块级别
        LogManager::getInstance().setModuleLogLevel(module, newLevel);
        
        // 保持旧的映射，用于向后兼容
        s_moduleLevels[module] = level;
    }
    
    /**
     * @brief 获取模块日志级别
     * @param module 模块名称
     * @return 日志级别
     */
    static LogLevel getModuleLogLevel(const std::string& module) {
        // 从统一日志系统获取级别
        Core::LogLevel level = LogManager::getInstance().getModuleLogLevel(module);
        
        // 转换回旧的日志级别
        return static_cast<LogLevel>(level);
    }
    
    /**
     * @brief 设置默认日志级别
     * @param level 日志级别
     */
    static void setDefaultLogLevel(LogLevel level) {
        // 在统一日志系统中设置全局级别
        LogManager::getInstance().setMinLogLevel(static_cast<Core::LogLevel>(level));
        
        // 保持旧的值，用于向后兼容
        s_defaultLevel = level;
    }
    
private:
    static std::unordered_map<std::string, LogLevel> s_moduleLevels;  ///< 模块日志级别映射
    static LogLevel s_defaultLevel;  ///< 默认日志级别
};

/**
 * @brief 日志异常类
 */
class LogException : public std::exception {
public:
    /**
     * @brief 构造函数
     * @param message 异常消息
     */
    LogException(const std::string& message) : m_message(message) {}
    
    /**
     * @brief 获取异常消息
     * @return 异常消息
     */
    const char* what() const noexcept override {
        return m_message.c_str();
    }
    
private:
    std::string m_message;  ///< 异常消息
};

} // namespace Core

// 初始化静态成员
namespace Core {
    std::unordered_map<std::string, LogLevel> LogConfig::s_moduleLevels;
    LogLevel LogConfig::s_defaultLevel = LogLevel::INFO;
}

// 调试级别定义
#define DEBUG_LEVEL_NONE 0    // 禁用所有调试输出
#define DEBUG_LEVEL_ERROR 1   // 仅错误和严重错误
#define DEBUG_LEVEL_WARNING 2 // 警告、错误和严重错误
#define DEBUG_LEVEL_INFO 3    // 信息、警告、错误和严重错误
#define DEBUG_LEVEL_DEBUG 4   // 所有调试输出

// 设置当前调试级别
#ifndef NDEBUG
#define CURRENT_DEBUG_LEVEL DEBUG_LEVEL_DEBUG
#else
#define CURRENT_DEBUG_LEVEL DEBUG_LEVEL_WARNING
#endif

// 流式调试宏 - 保持原有名称，但使用新的统一日志系统
#define DEBUG_STREAM() Core::DebugStream(Core::LogLevel::DEBUG, __FILE__, __LINE__)
#define INFO_STREAM() Core::DebugStream(Core::LogLevel::INFO, __FILE__, __LINE__)
#define WARNING_STREAM() Core::DebugStream(Core::LogLevel::WARNING, __FILE__, __LINE__)
#define ERROR_STREAM() Core::DebugStream(Core::LogLevel::ERROR, __FILE__, __LINE__)
#define CRITICAL_STREAM() Core::DebugStream(Core::LogLevel::CRITICAL, __FILE__, __LINE__)

// 条件调试宏 - 保持原有名称，但使用新的统一日志系统
#define DEBUG_LOG(message) \
    if (CURRENT_DEBUG_LEVEL >= DEBUG_LEVEL_DEBUG) { \
        Core::LogManager::getInstance().debug(std::string(message), {__FILE__, __LINE__}); \
    }

#define INFO_LOG(message) \
    if (CURRENT_DEBUG_LEVEL >= DEBUG_LEVEL_INFO) { \
        Core::LogManager::getInstance().info(std::string(message), {__FILE__, __LINE__}); \
    }

#define WARNING_LOG(message) \
    if (CURRENT_DEBUG_LEVEL >= DEBUG_LEVEL_WARNING) { \
        Core::LogManager::getInstance().warning(std::string(message), {__FILE__, __LINE__}); \
    }

#define ERROR_LOG(message) \
    if (CURRENT_DEBUG_LEVEL >= DEBUG_LEVEL_ERROR) { \
        Core::LogManager::getInstance().error(std::string(message), {__FILE__, __LINE__}); \
    }

#define CRITICAL_LOG(message) \
    if (CURRENT_DEBUG_LEVEL >= DEBUG_LEVEL_ERROR) { \
        Core::LogManager::getInstance().critical(std::string(message), {__FILE__, __LINE__}); \
    }

// 模块调试宏
#define MODULE_DEBUG_ENABLED(module) \
    (Core::LogConfig::getModuleLogLevel(module) >= Core::LogLevel::DEBUG)

#define MODULE_DEBUG_LOG(module, message) \
    if (MODULE_DEBUG_ENABLED(module)) { \
        Core::LogManager::getInstance().debug(std::string(message), {__FILE__, __LINE__, "", module}); \
    }

// 断言宏
#ifndef NDEBUG
#define LOG_ASSERT(condition) \
    if (!(condition)) { \
        Core::LogManager::getInstance().critical( \
            std::string("Assertion failed: ") + #condition, \
            {__FILE__, __LINE__}); \
        assert(condition); \
    }
#else
#define LOG_ASSERT(condition) ((void)0)
#endif

#ifndef NDEBUG
#define LOG_ASSERT_X(condition, message) \
    if (!(condition)) { \
        Core::LogManager::getInstance().critical( \
            std::string("Assertion failed: ") + #condition + \
            " - " + message, \
            {__FILE__, __LINE__}); \
        assert(condition); \
    }
#else
#define LOG_ASSERT_X(condition, message) ((void)0)
#endif

#ifndef NDEBUG
#define CHECK_PTR(ptr) \
    if ((ptr) == nullptr) { \
        Core::LogManager::getInstance().critical( \
            std::string("Null pointer: ") + #ptr, \
            {__FILE__, __LINE__}); \
        assert((ptr) != nullptr); \
    }
#else
#define CHECK_PTR(ptr) ((void)0)
#endif

// 发布模式断言
#define RELEASE_ASSERT(condition, message) \
    if (!(condition)) { \
        Core::LogManager::getInstance().critical( \
            std::string("Assertion failed: ") + #condition + \
            " - " + message, \
            {__FILE__, __LINE__}); \
    }

// 致命错误处理
#define FATAL_ERROR(message) \
    { \
        Core::LogManager::getInstance().critical( \
            std::string("FATAL ERROR: ") + message, \
            {__FILE__, __LINE__}); \
        std::terminate(); \
    }

// 异常捕获和日志记录
#define TRY_LOG_CATCH(code) \
    try { \
        code; \
    } catch (const std::exception& e) { \
        Core::LogManager::getInstance().error( \
            std::string("Exception caught: ") + e.what(), \
            {__FILE__, __LINE__}); \
    } catch (...) { \
        Core::LogManager::getInstance().error( \
            std::string("Unknown exception caught"), \
            {__FILE__, __LINE__}); \
    }

// 错误代码检查和日志记录
#define CHECK_ERROR_CODE(code, message) \
    if ((code) != 0) { \
        Core::LogManager::getInstance().error( \
            std::string(message) + ": error code " + std::to_string(code), \
            {__FILE__, __LINE__}); \
        return false; \
    }

// 资源清理宏
#define WITH_RESOURCE(resource, cleanup) \
    for (bool _done = false; !_done; _done = true) \
        for (auto _resource = (resource); !_done; (_done = true, cleanup))

// Qt调试操作替换宏
#define qDebug() DEBUG_STREAM()
#define qInfo() INFO_STREAM()
#define qWarning() WARNING_STREAM()
#define qCritical() CRITICAL_STREAM()
#define qFatal(message) FATAL_ERROR(message)

// Qt断言替换宏
#define Q_ASSERT(condition) LOG_ASSERT(condition)
#define Q_ASSERT_X(condition, where, message) LOG_ASSERT_X(condition, std::string(where) + ": " + std::string(message))
#define Q_CHECK_PTR(ptr) CHECK_PTR(ptr)

#endif // CORE_DEBUGUTILS_H