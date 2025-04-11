#ifndef CORE_TIMEUTILS_H
#define CORE_TIMEUTILS_H

#include <chrono>
#include <string>
#include <sstream>
#include <iomanip>
#include <unordered_map>
#include <stdexcept>
#include <filesystem>

namespace Core {

// 时间相关常量
namespace TimeConstants {
    using Precision = std::chrono::microseconds;
    constexpr auto MinTime = std::chrono::system_clock::time_point::min();
    constexpr auto MaxTime = std::chrono::system_clock::time_point::max();
}

// 时间异常类
class TimeException : public std::runtime_error {
public:
    enum class ErrorCode {
        InvalidTime,
        OutOfRange,
        ParseError,
        FormatError,
        TimezoneError
    };
    
    TimeException(ErrorCode code, const std::string& message)
        : std::runtime_error(message), code_(code) {}
    
    ErrorCode getCode() const { return code_; }
    
private:
    ErrorCode code_;
};

// 时间转换工具类
class TimeConverter {
public:
    // 从时间戳创建时间点
    static std::chrono::system_clock::time_point fromTimestamp(std::time_t timestamp) {
        return std::chrono::system_clock::from_time_t(timestamp);
    }
    
    // 转换为时间戳
    static std::time_t toTimestamp(const std::chrono::system_clock::time_point& tp) {
        return std::chrono::system_clock::to_time_t(tp);
    }
    
    // 从文件时间转换
    static std::chrono::system_clock::time_point fromFileTime(
            const std::filesystem::file_time_type& ft) {
        // 由于std::chrono::clock_cast在C++20中才引入，我们使用自定义实现
        // 将文件时间转换为系统时间
        auto ft_duration = ft.time_since_epoch();
        auto sys_duration = std::chrono::duration_cast<std::chrono::system_clock::duration>(ft_duration);
        return std::chrono::system_clock::time_point(sys_duration);
    }
    
    // 转换为文件时间
    static std::filesystem::file_time_type toFileTime(
            const std::chrono::system_clock::time_point& tp) {
        // 由于std::chrono::clock_cast在C++20中才引入，我们使用自定义实现
        // 将系统时间转换为文件时间
        auto sys_duration = tp.time_since_epoch();
        auto ft_duration = std::chrono::duration_cast<std::filesystem::file_time_type::duration>(sys_duration);
        return std::filesystem::file_time_type(ft_duration);
    }
};

// 时间格式化工具类
class TimeFormatter {
public:
    // 格式化为字符串
    static std::string format(const std::chrono::system_clock::time_point& tp,
                            const std::string& fmt = "%Y-%m-%d %H:%M:%S") {
        auto time = TimeConverter::toTimestamp(tp);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time), fmt.c_str());
        return ss.str();
    }
    
    // 从字符串解析
    static std::chrono::system_clock::time_point parse(
            const std::string& str,
            const std::string& fmt = "%Y-%m-%d %H:%M:%S") {
        std::tm tm = {};
        std::stringstream ss(str);
        ss >> std::get_time(&tm, fmt.c_str());
        if (ss.fail()) {
            throw TimeException(TimeException::ErrorCode::ParseError,
                              "Failed to parse time string: " + str);
        }
        return TimeConverter::fromTimestamp(std::mktime(&tm));
    }
};

// 时区处理工具类
class TimeZoneHandler {
public:
    // 转换为UTC
    static std::chrono::system_clock::time_point toUTC(
            const std::chrono::system_clock::time_point& local) {
        auto time = TimeConverter::toTimestamp(local);
        std::tm tm = *std::gmtime(&time);
        return TimeConverter::fromTimestamp(std::mktime(&tm));
    }
    
    // 转换为本地时间
    static std::chrono::system_clock::time_point toLocal(
            const std::chrono::system_clock::time_point& utc) {
        auto time = TimeConverter::toTimestamp(utc);
        std::tm tm = *std::localtime(&time);
        return TimeConverter::fromTimestamp(std::mktime(&tm));
    }
    
    // 获取时区偏移
    static std::chrono::seconds getTimezoneOffset() {
        std::time_t now = std::time(nullptr);
        std::tm local = *std::localtime(&now);
        std::tm utc = *std::gmtime(&now);
        return std::chrono::seconds(std::mktime(&local) - std::mktime(&utc));
    }
};

// 精度处理工具类
class PrecisionHandler {
public:
    // 标准化时间精度
    template<typename Duration>
    static std::chrono::system_clock::time_point normalize(
            const std::chrono::system_clock::time_point& tp) {
        auto duration = tp.time_since_epoch();
        return std::chrono::system_clock::time_point(
            std::chrono::duration_cast<Duration>(duration));
    }
    
    // 检查精度是否足够
    template<typename Duration>
    static bool hasSufficientPrecision(const Duration& d) {
        return d >= TimeConstants::Precision(1);
    }
    
    // 验证时间点
    static bool validateTimePoint(const std::chrono::system_clock::time_point& tp) {
        return tp != TimeConstants::MinTime && tp != TimeConstants::MaxTime;
    }
};

// 时间缓存工具类
class TimeCache {
public:
    // 缓存最近的时间转换
    static std::chrono::system_clock::time_point getCachedTime(std::time_t timestamp) {
        static std::unordered_map<std::time_t, std::chrono::system_clock::time_point> cache;
        static constexpr std::size_t maxCacheSize = 1000;
        
        auto it = cache.find(timestamp);
        if (it != cache.end()) {
            return it->second;
        }
        
        if (cache.size() >= maxCacheSize) {
            cache.clear();
        }
        
        auto tp = TimeConverter::fromTimestamp(timestamp);
        cache[timestamp] = tp;
        return tp;
    }
};

// 批量处理工具类
class TimeBatchProcessor {
public:
    // 批量转换时间
    template<typename Container>
    static Container batchConvert(const Container& timestamps) {
        Container result;
        result.reserve(timestamps.size());
        
        for (const auto& ts : timestamps) {
            result.push_back(TimeConverter::fromTimestamp(ts));
        }
        
        return result;
    }
};

} // namespace Core

#endif // CORE_TIMEUTILS_H 