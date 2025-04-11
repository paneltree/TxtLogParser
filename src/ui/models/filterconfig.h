#ifndef FILTERCONFIG_H
#define FILTERCONFIG_H

#include <QString>
#include <QColor>

class FilterConfig {
public:
    qint32 filterId;
    qint32 filterRow;
    QString filterPattern;
    bool caseSensitive;
    bool wholeWord;
    bool isRegex;
    QColor color;
    bool enabled;
    
    // Match tracking
    int matchCount;
    int currentMatchPosition;

    // Default constructor
    FilterConfig()
        : filterId(-1), filterRow(-1), filterPattern(""), caseSensitive(false), wholeWord(false), isRegex(false),
          color(QColor("#FF4444")), enabled(true), matchCount(0), currentMatchPosition(0) {}
    
    // Regular constructor
    FilterConfig(int32_t id, int32_t row, const QString &pattern, bool caseSens, bool whole, bool regex, bool enable, const QColor &col)
        : filterId(id), filterRow(row), filterPattern(pattern), caseSensitive(caseSens), wholeWord(whole), isRegex(regex),
          color(col), enabled(enable), matchCount(0), currentMatchPosition(0) {}
};

#endif // FILTERCONFIG_H 