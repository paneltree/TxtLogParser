#ifndef FILEADAPTER_H
#define FILEADAPTER_H

#include <QList>
#include <QString>
#include <memory>
#include "../ui/models/fileinfo.h"
#include "../core/FileData.h"

/**
 * @brief Adapter class to bridge between Qt UI (FileInfo) and core logic (Core::FileData)
 * 
 * This class provides methods to convert between FileInfo and Core::FileData,
 * allowing the UI to work with Qt types while the core logic uses pure C++ types.
 */
class FileAdapter {
public:
    // Singleton instance
    static FileAdapter& getInstance();
    
    // Convert from FileInfo to Core::FileData
    Core::FileDataPtr toFileData(const FileInfo& fileInfo);
    
    // Convert from Core::FileData to FileInfo
    FileInfo toFileInfo(const Core::FileData& data);

    bool toFileInfo(const Core::FileDataPtr& data, FileInfo& info);
    
    // Convert a list of FileInfo to a vector of Core::FileDataPtr
    std::vector<Core::FileDataPtr> toFileDataList(const QList<FileInfo>& fileInfoList);
    
    // Convert a vector of Core::FileDataPtr to a list of FileInfo
    QList<FileInfo> toFileInfoList(const std::vector<Core::FileDataPtr>& dataList);
    
private:
    // Private constructor for singleton
    FileAdapter() = default;
    
    // Prevent copying
    FileAdapter(const FileAdapter&) = delete;
    FileAdapter& operator=(const FileAdapter&) = delete;
};

#endif // FILEADAPTER_H 