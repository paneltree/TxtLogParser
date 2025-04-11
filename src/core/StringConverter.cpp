#include "StringConverter.h"

namespace Core {

std::string StringConverter::fromQString(const QString& qstr) {
    return qstr.toStdString();
}

QString StringConverter::toQString(const std::string& str) {
    return QString::fromStdString(str);
}

} // namespace Core 