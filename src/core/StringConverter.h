#ifndef STRINGCONVERTER_H
#define STRINGCONVERTER_H

#include <string>
#include <QString>

namespace Core {

/**
 * @brief 字符串转换工具类，用于处理QString和std::string之间的转换
 */
class StringConverter {
public:
    /**
     * @brief 将QString转换为std::string
     * @param qstr 要转换的QString
     * @return 转换后的std::string
     */
    static std::string fromQString(const QString& qstr);
    
    /**
     * @brief 将std::string转换为QString
     * @param str 要转换的std::string
     * @return 转换后的QString
     */
    static QString toQString(const std::string& str);
};

} // namespace Core

#endif // STRINGCONVERTER_H 