#ifndef FILEINFO_H
#define FILEINFO_H

#include <QString>
#include <chrono>

class FileInfo {
public:
    QString filePath;
    QString fileName;
    std::chrono::system_clock::time_point modifiedDate;
    qint64 fileSize;
    bool isSelected;
    qint32 fileId;
    qint32 fileRow;
    bool isExists;

    FileInfo();
    FileInfo(const QString &path, const QString &name, 
             const std::chrono::system_clock::time_point &modified,
             qint64 size, bool isSelected = true, qint32 id = -1, qint32 row = -1, bool isExists = true);
};

#endif // FILEINFO_H 