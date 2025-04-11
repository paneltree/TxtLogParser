#include "FileAdapter.h"
#include "../core/TimeUtils.h"
#include "../core/FileSystem.h"

// Singleton instance
FileAdapter& FileAdapter::getInstance() {
    static FileAdapter instance;
    return instance;
}

// Convert from FileInfo to Core::FileData
Core::FileDataPtr FileAdapter::toFileData(const FileInfo& fileInfo) {
    // Create a new FileData with the path
    auto fileData = std::make_shared<Core::FileData>(fileInfo.filePath.toStdString());
    
    // Convert system_clock::time_point to time_t
    time_t modTime = Core::TimeConverter::toTimestamp(fileInfo.modifiedDate);
    
    // Set additional properties
    fileData->setFileName(fileInfo.fileName.toStdString());
    fileData->setModifiedTime(modTime);
    fileData->setFileSize(fileInfo.fileSize);
    fileData->setSelected(fileInfo.isSelected);
    fileData->setFileId(fileInfo.fileId);
    
    return fileData;
}

// Convert from Core::FileData to FileInfo
FileInfo FileAdapter::toFileInfo(const Core::FileData& data) {
    std::filesystem::path fsPath = Core::PathConverter::fromString(data.getFilePath());
    
    // Get file information using std::filesystem
    auto modTime = Core::FileSystem::lastWriteTime(fsPath);
    auto size = Core::FileSystem::fileSize(fsPath);
    
    // Convert file_time_type to system_clock::time_point
    auto systemTime = Core::TimeConverter::fromFileTime(modTime);
    
    // Create FileInfo with all properties
    return FileInfo(
        QString::fromStdString(data.getFilePath()),
        QString::fromStdString(data.getFileName()),
        systemTime,
        static_cast<qint64>(size),
        data.isSelected(),
        data.getFileId(),
        data.getFileRow(),
        data.isExists()
    );
}

bool FileAdapter::toFileInfo(const Core::FileDataPtr& data, FileInfo& info)
{
    if (data) {
        info = toFileInfo(*data);
        return true;
    }
    
    return false;
}

// Convert a list of FileInfo to a vector of Core::FileDataPtr
std::vector<Core::FileDataPtr> FileAdapter::toFileDataList(const QList<FileInfo>& fileInfoList) {
    std::vector<Core::FileDataPtr> dataList;
    dataList.reserve(fileInfoList.size());
    
    for (const auto& fileInfo : fileInfoList) {
        dataList.push_back(toFileData(fileInfo));
    }
    
    return dataList;
}

// Convert a vector of Core::FileDataPtr to a list of FileInfo
QList<FileInfo> FileAdapter::toFileInfoList(const std::vector<Core::FileDataPtr>& dataList) {
    QList<FileInfo> fileInfoList;
    
    for (const auto& data : dataList) {
        if (data) {
            fileInfoList.append(toFileInfo(*data));
        }
    }
    
    return fileInfoList;
} 