#ifndef CORE_JSONUTILS_H
#define CORE_JSONUTILS_H

#include <string>
#include <stdexcept>
#include <regex>
#include <fstream>
#include <nlohmann/json.hpp>

namespace Core {

class JsonException : public std::runtime_error {
public:
    explicit JsonException(const std::string& message) 
        : std::runtime_error(message) {}
};

class JsonUtils {
public:
    // 解析JSON字符串
    static nlohmann::json parse(const std::string& input) {
        try {
            return nlohmann::json::parse(input);
        } catch (const nlohmann::json::parse_error& e) {
            throw JsonException("JSON解析错误: " + std::string(e.what()));
        } catch (const std::exception& e) {
            throw JsonException("JSON处理错误: " + std::string(e.what()));
        }
    }
    
    // 序列化JSON到字符串
    static std::string stringify(const nlohmann::json& j, int indent = 4) {
        try {
            return j.dump(indent);
        } catch (const std::exception& e) {
            throw JsonException("JSON序列化错误: " + std::string(e.what()));
        }
    }
    
    // 类型检查
    template<typename T>
    static bool checkType(const nlohmann::json& j) {
        if constexpr (std::is_same_v<T, std::string>) {
            return j.is_string();
        } else if constexpr (std::is_integral_v<T>) {
            return j.is_number_integer();
        } else if constexpr (std::is_floating_point_v<T>) {
            return j.is_number_float();
        } else if constexpr (std::is_same_v<T, bool>) {
            return j.is_boolean();
        }
        return false;
    }
    
    // 值范围验证
    template<typename T>
    static bool validateRange(const T& value, const T& min, const T& max) {
        return value >= min && value <= max;
    }
    
    // 字符串格式验证
    static bool validateString(const std::string& value, const std::string& pattern) {
        try {
            std::regex re(pattern);
            return std::regex_match(value, re);
        } catch (const std::regex_error& e) {
            throw JsonException("正则表达式错误: " + std::string(e.what()));
        }
    }
    
    // 安全获取值
    template<typename T>
    static T getValue(const nlohmann::json& j, const std::string& key, const T& defaultValue) {
        try {
            return j.contains(key) ? j[key].get<T>() : defaultValue;
        } catch (const std::exception& e) {
            return defaultValue;
        }
    }
    
    // 检查必需字段
    static bool checkRequiredFields(const nlohmann::json& j, 
                                  const std::vector<std::string>& fields) {
        for (const auto& field : fields) {
            if (!j.contains(field)) {
                return false;
            }
        }
        return true;
    }
    
    // 从文件加载JSON
    static nlohmann::json loadFromFile(const std::string& filePath) {
        try {
            std::ifstream file(filePath);
            if (!file.is_open()) {
                throw JsonException("无法打开文件: " + filePath);
            }
            return nlohmann::json::parse(file);
        } catch (const nlohmann::json::parse_error& e) {
            throw JsonException("JSON文件解析错误: " + std::string(e.what()));
        } catch (const std::exception& e) {
            throw JsonException("文件读取错误: " + std::string(e.what()));
        }
    }
    
    // 保存JSON到文件
    static void saveToFile(const std::string& filePath, 
                          const nlohmann::json& j,
                          int indent = 4) {
        try {
            std::ofstream file(filePath);
            if (!file.is_open()) {
                throw JsonException("无法创建文件: " + filePath);
            }
            file << j.dump(indent);
        } catch (const std::exception& e) {
            throw JsonException("文件写入错误: " + std::string(e.what()));
        }
    }
};

} // namespace Core

#endif // CORE_JSONUTILS_H 