#include "DebugUtils.h"
#include "StringConverter.h"
#include <QColor>
#include <QString>
#include <QList>
#include <QMap>
#include <QVector>
#include <QStringList>
#include <QDateTime>
#include <QUrl>
#include <QRect>
#include <QPoint>
#include <QSize>

namespace Core {

// 特化版本，处理QString
template<>
DebugStream& DebugStream::operator<<(const QString& value) {
    m_stream << StringConverter::fromQString(value);
    return *this;
}

// 特化版本，处理QColor
template<>
DebugStream& DebugStream::operator<<(const QColor& value) {
    m_stream << "QColor(" 
             << value.red() << ", " 
             << value.green() << ", " 
             << value.blue();
    
    if (value.alpha() < 255) {
        m_stream << ", " << value.alpha();
    }
    
    m_stream << ")";
    return *this;
}

// 特化版本，处理QDateTime
template<>
DebugStream& DebugStream::operator<<(const QDateTime& value) {
    if (value.isValid()) {
        m_stream << StringConverter::fromQString(value.toString(Qt::ISODate));
    } else {
        m_stream << "Invalid QDateTime";
    }
    return *this;
}

// 特化版本，处理QUrl
template<>
DebugStream& DebugStream::operator<<(const QUrl& value) {
    m_stream << StringConverter::fromQString(value.toString());
    return *this;
}

// 特化版本，处理QRect
template<>
DebugStream& DebugStream::operator<<(const QRect& value) {
    m_stream << "QRect(" 
             << value.x() << ", " 
             << value.y() << ", " 
             << value.width() << ", " 
             << value.height() << ")";
    return *this;
}

// 特化版本，处理QPoint
template<>
DebugStream& DebugStream::operator<<(const QPoint& value) {
    m_stream << "QPoint(" << value.x() << ", " << value.y() << ")";
    return *this;
}

// 特化版本，处理QSize
template<>
DebugStream& DebugStream::operator<<(const QSize& value) {
    m_stream << "QSize(" << value.width() << ", " << value.height() << ")";
    return *this;
}

// 特化版本，处理QStringList
template<>
DebugStream& DebugStream::operator<<(const QStringList& value) {
    m_stream << "QStringList(";
    bool first = true;
    for (const auto& str : value) {
        if (!first) {
            m_stream << ", ";
        }
        first = false;
        *this << str;
    }
    m_stream << ")";
    return *this;
}

// 通用模板，处理QList<T>
template<typename T>
DebugStream& operator<<(DebugStream& stream, const QList<T>& list) {
    stream << "QList(";
    bool first = true;
    for (const auto& item : list) {
        if (!first) {
            stream << ", ";
        }
        first = false;
        stream << item;
    }
    stream << ")";
    return stream;
}

// 通用模板，处理QVector<T>
template<typename T>
DebugStream& operator<<(DebugStream& stream, const QVector<T>& vector) {
    stream << "QVector(";
    bool first = true;
    for (const auto& item : vector) {
        if (!first) {
            stream << ", ";
        }
        first = false;
        stream << item;
    }
    stream << ")";
    return stream;
}

// 通用模板，处理QMap<K, V>
template<typename K, typename V>
DebugStream& operator<<(DebugStream& stream, const QMap<K, V>& map) {
    stream << "QMap(";
    bool first = true;
    for (auto it = map.begin(); it != map.end(); ++it) {
        if (!first) {
            stream << ", ";
        }
        first = false;
        stream << it.key() << ": " << it.value();
    }
    stream << ")";
    return stream;
}

} // namespace Core 