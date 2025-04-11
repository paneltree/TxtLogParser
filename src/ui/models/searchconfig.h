#ifndef SEARCHCONFIG_H
#define SEARCHCONFIG_H

#include <QString>
#include <QColor>

class SearchConfig {
public:
    qint32 searchId;
    qint32 searchRow;
    QString searchPattern;
    bool caseSensitive;
    bool wholeWord;
    bool isRegex;
    QColor color;
    bool enabled;
    
    // Match tracking
    int matchCount;
    int currentMatchPosition;

    // Default constructor
    SearchConfig()
        : searchId(-1), searchRow(-1), searchPattern(""), caseSensitive(false), wholeWord(false), isRegex(false),
          color(QColor("#FF4444")), enabled(true), matchCount(0), currentMatchPosition(0) {}
    
    // Regular constructor
    SearchConfig(int32_t id, int32_t row, const QString &pattern, bool caseSens, bool whole, bool regex, bool enable, const QColor &col)
        : searchId(id), searchRow(row), searchPattern(pattern), caseSensitive(caseSens), wholeWord(whole), isRegex(regex),
          color(col), enabled(enable), matchCount(0), currentMatchPosition(0) {}
};

#endif // SEARCHCONFIG_H 