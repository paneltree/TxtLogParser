#include "fileinfo.h"

FileInfo::FileInfo(){
    
}
FileInfo::FileInfo(const QString &path, const QString &name,
                  const std::chrono::system_clock::time_point &modified,
                  qint64 size, bool isSelected, int id, int row, bool isExists)
    : filePath(path)
    , fileName(name)
    , modifiedDate(modified)
    , fileSize(size)
    , isSelected(isSelected)
    , fileId(id)
    , fileRow(row)
    , isExists(isExists)

{
} 