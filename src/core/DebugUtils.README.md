# DebugUtils 使用指南

## 概述

`DebugUtils` 是一个用于替代Qt调试操作的工具库，提供了标准C++实现的调试、日志记录、断言和错误处理功能。本库设计用于减少对Qt库的依赖，同时提供更强大、更灵活的调试功能。

## 主要功能

1. **流式调试输出**：替代 `QDebug` 的流式语法
2. **条件调试宏**：根据调试级别输出日志
3. **模块级日志控制**：为不同模块设置不同的日志级别
4. **断言宏**：替代Qt断言，提供更丰富的断言功能
5. **异常处理**：简化异常捕获和日志记录
6. **错误代码处理**：简化错误代码检查和日志记录
7. **资源清理**：确保资源正确释放

## 使用方法

### 1. 流式调试输出

```cpp
// 使用流式调试宏
DEBUG_STREAM() << "这是一个调试消息，包含整数: " << 42 << " 和浮点数: " << 3.14;
INFO_STREAM() << "这是一个信息消息";
WARNING_STREAM() << "这是一个警告消息";
ERROR_STREAM() << "这是一个错误消息";
CRITICAL_STREAM() << "这是一个严重错误消息";

// 使用Qt兼容宏（内部调用上述宏）
qDebug() << "这是使用qDebug()的消息";
qInfo() << "这是使用qInfo()的消息";
qWarning() << "这是使用qWarning()的消息";
qCritical() << "这是使用qCritical()的消息";
```

### 2. 条件调试宏

```cpp
// 根据当前调试级别输出日志
DEBUG_LOG("这是一个条件调试消息");
INFO_LOG("这是一个条件信息消息");
WARNING_LOG("这是一个条件警告消息");
ERROR_LOG("这是一个条件错误消息");
CRITICAL_LOG("这是一个条件严重错误消息");
```

### 3. 模块级日志控制

```cpp
// 设置模块日志级别
Core::LogConfig::setModuleLogLevel("UI", Core::LogLevel::INFO);
Core::LogConfig::setModuleLogLevel("Network", Core::LogLevel::DEBUG);
Core::LogConfig::setModuleLogLevel("Database", Core::LogLevel::WARNING);

// 使用模块调试宏
MODULE_DEBUG_LOG("UI", "这是UI模块的调试消息");
MODULE_DEBUG_LOG("Network", "这是Network模块的调试消息");
MODULE_DEBUG_LOG("Database", "这是Database模块的调试消息");
```

### 4. 断言宏

```cpp
// 基本断言（仅在调试模式下启用）
LOG_ASSERT(condition);

// 带消息的断言
LOG_ASSERT_X(condition, "断言失败的详细信息");

// 指针检查
CHECK_PTR(pointer);

// 发布模式断言（即使在发布模式下也会记录日志，但不会终止程序）
RELEASE_ASSERT(condition, "断言失败的详细信息");

// Qt兼容断言宏
Q_ASSERT(condition);
Q_ASSERT_X(condition, "where", "message");
Q_CHECK_PTR(pointer);
```

### 5. 异常处理

```cpp
// 异常捕获和日志记录
TRY_LOG_CATCH({
    // 可能抛出异常的代码
    functionThatMayThrow();
});

// 抛出日志异常
throw Core::LogException("发生了错误");
```

### 6. 错误代码处理

```cpp
// 错误代码检查和日志记录
int result = someFunction();
CHECK_ERROR_CODE(result, "操作失败");
```

### 7. 资源清理

```cpp
// 确保资源正确释放
FILE* file = nullptr;
WITH_RESOURCE(fopen("file.txt", "r"), fclose(_resource)) {
    if (_resource) {
        // 文件操作...
    }
}
```

## 调试级别

可以通过定义 `CURRENT_DEBUG_LEVEL` 来控制全局调试级别：

```cpp
// 调试级别定义
#define DEBUG_LEVEL_NONE 0    // 禁用所有调试输出
#define DEBUG_LEVEL_ERROR 1   // 仅错误和严重错误
#define DEBUG_LEVEL_WARNING 2 // 警告、错误和严重错误
#define DEBUG_LEVEL_INFO 3    // 信息、警告、错误和严重错误
#define DEBUG_LEVEL_DEBUG 4   // 所有调试输出

// 在调试模式下，默认使用DEBUG级别；在发布模式下，默认使用WARNING级别
#ifndef NDEBUG
#define CURRENT_DEBUG_LEVEL DEBUG_LEVEL_DEBUG
#else
#define CURRENT_DEBUG_LEVEL DEBUG_LEVEL_WARNING
#endif
```

## 与Qt调试操作的对应关系

| Qt调试操作 | DebugUtils替代方案 |
|------------|-------------------|
| QDebug     | Core::DebugStream |
| qDebug()   | DEBUG_STREAM() 或 qDebug() |
| qInfo()    | INFO_STREAM() 或 qInfo() |
| qWarning() | WARNING_STREAM() 或 qWarning() |
| qCritical()| CRITICAL_STREAM() 或 qCritical() |
| qFatal()   | FATAL_ERROR() 或 qFatal() |
| Q_ASSERT   | LOG_ASSERT() 或 Q_ASSERT() |
| Q_ASSERT_X | LOG_ASSERT_X() 或 Q_ASSERT_X() |
| Q_CHECK_PTR| CHECK_PTR() 或 Q_CHECK_PTR() |

## 性能考虑

1. 条件调试宏使用预处理器条件，在不满足条件时不会有性能开销
2. 断言宏在发布模式下会被禁用，不会影响性能
3. 流式调试输出使用 `std::stringstream`，仅在实际输出时才会格式化字符串
4. 模块级日志控制允许精细控制不同模块的日志级别，减少不必要的日志输出

## 最佳实践

1. 使用 `DEBUG_STREAM()` 和 `INFO_STREAM()` 记录详细的调试信息
2. 使用 `WARNING_STREAM()` 记录可能的问题
3. 使用 `ERROR_STREAM()` 和 `CRITICAL_STREAM()` 记录错误
4. 使用 `LOG_ASSERT()` 和 `CHECK_PTR()` 验证关键条件
5. 使用 `TRY_LOG_CATCH()` 捕获和记录异常
6. 使用 `MODULE_DEBUG_LOG()` 控制不同模块的日志级别
7. 在性能关键的代码中，使用条件调试宏而不是流式调试宏 