#ifndef QOUTPUTLINE_H
#define QOUTPUTLINE_H

#include <QObject>
#include <QList>

class QOutputSubLine 
{
    public:
    qint32 m_fileId;
    QString m_content;
    QString m_color;
};

class QOutputLine{
    public:
    qint32 m_fileId;
    qint32 m_fileRow;
    qint32 m_lineIndex;
    QList<QOutputSubLine> m_subLines;
};

#endif // QOUTPUTLINE_H
