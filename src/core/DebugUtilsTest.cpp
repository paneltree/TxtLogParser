#include "DebugUtils.h"
#include "StringConverter.h"
#include <iostream>
#include <vector>
#include <map>
#include <QString>
#include <QList>
#include <QMap>
#include <QDateTime>

/**
 * @brief 测试DebugUtils功能的函数
 * 
 * 此函数用于测试DebugUtils中实现的各种调试功能，包括：
 * - 流式调试宏
 * - 条件调试宏
 * - 断言宏
 * - 异常处理宏
 * - 错误代码处理宏
 */
void testDebugUtils() {
    // 测试流式调试宏
    DEBUG_STREAM() << "这是一个调试消息，包含整数: " << 42 << " 和浮点数: " << 3.14;
    INFO_STREAM() << "这是一个信息消息，包含字符串: " << "Hello World";
    WARNING_STREAM() << "这是一个警告消息，包含布尔值: " << true;
    ERROR_STREAM() << "这是一个错误消息，包含多个值: " << 100 << ", " << "Error" << ", " << 2.718;
    
    // 测试Qt类型的流式输出
    QString qstr = "Qt字符串";
    QList<int> qlist = {1, 2, 3, 4, 5};
    QMap<QString, int> qmap;
    qmap["one"] = 1;
    qmap["two"] = 2;
    qmap["three"] = 3;
    
    DEBUG_STREAM() << "Qt类型测试 - QString: " << qstr;
    DEBUG_STREAM() << "Qt类型测试 - QList: " << qlist;
    DEBUG_STREAM() << "Qt类型测试 - QMap: " << qmap;
    DEBUG_STREAM() << "Qt类型测试 - QDateTime: " << QDateTime::currentDateTime();
    
    // 测试条件调试宏
    DEBUG_LOG("这是一个条件调试消息");
    INFO_LOG("这是一个条件信息消息");
    WARNING_LOG("这是一个条件警告消息");
    ERROR_LOG("这是一个条件错误消息");
    
    // 测试模块调试宏
    Core::LogConfig::setModuleLogLevel("TestModule", Core::LogLevel::DEBUG);
    MODULE_DEBUG_LOG("TestModule", "这是一个模块调试消息");
    
    // 测试断言宏（注意：这些断言在发布模式下会被禁用）
    LOG_ASSERT(true); // 这个断言会通过
    LOG_ASSERT_X(1 + 1 == 2, "这个断言会通过，并带有消息");
    
    // 测试指针检查
    int* ptr = new int(42);
    CHECK_PTR(ptr);
    delete ptr;
    
    // 测试发布模式断言
    RELEASE_ASSERT(true, "这个断言会通过，即使在发布模式下");
    
    // 测试异常捕获和日志记录
    TRY_LOG_CATCH({
        // 模拟可能抛出异常的代码
        std::vector<int> vec;
        // 下面的代码在某些情况下可能会抛出异常
        // vec.at(10) = 42; // 这会抛出std::out_of_range异常
    });
    
    // 测试错误代码处理
    int errorCode = 0; // 0表示成功
    CHECK_ERROR_CODE(errorCode, "操作失败");
    
    // 测试资源清理
    FILE* file = nullptr;
    WITH_RESOURCE(fopen("test.txt", "r"), fclose(_resource)) {
        // 如果文件成功打开，这里的代码会执行
        // 无论如何，fclose都会在最后被调用
        if (_resource) {
            // 文件操作...
        }
    }
    
    // 测试Qt调试操作替换宏
    qDebug() << "这是使用qDebug()的消息，实际上调用了DEBUG_STREAM()";
    qInfo() << "这是使用qInfo()的消息，实际上调用了INFO_STREAM()";
    qWarning() << "这是使用qWarning()的消息，实际上调用了WARNING_STREAM()";
    qCritical() << "这是使用qCritical()的消息，实际上调用了CRITICAL_STREAM()";
    
    // 测试Qt断言替换宏
    Q_ASSERT(true); // 这个断言会通过
    Q_ASSERT_X(1 + 1 == 2, "Math", "这个断言会通过，并带有消息");
    Q_CHECK_PTR(ptr); // 这个指针检查会通过
    
    std::cout << "DebugUtils测试完成" << std::endl;
}

// 注意：此文件仅用于测试，不会在实际构建中使用
// 要运行测试，可以创建一个单独的测试程序，或者在适当的地方调用testDebugUtils()函数 