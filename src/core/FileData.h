#ifndef FILEDATA_H
#define FILEDATA_H

#include <string>
#include <vector>
#include <memory>
#include <ctime>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace Core {

/**
 * @brief Pure C++ class representing file data
 * 
 * This class replaces the Qt-dependent FileInfo class with a pure C++ implementation.
 */
class FileData {
public:
    FileData() = default;
    FileData(const std::string& path) : m_filePath(path) {}
    FileData(const std::string& path, const std::string& name, time_t modified = 0, 
             int64_t size = 0, bool sel = false, int id = -1) 
        : m_filePath(path), m_fileName(name), m_modifiedTime(modified), 
          m_fileSize(size), m_selected(sel), m_id(id), m_isExists(false) {}

    bool init(int32_t id, int32_t fileRow, const std::string& path);

    bool saveToJson(json& j) const;
    bool loadFromJson(const json& j);


    int32_t getFileId() const { return m_id; }
    void setFileId(int32_t id) { m_id = id; }

    int32_t getFileRow() const { return m_fileRow; }
    void setFileRow(int32_t fileRow) { m_fileRow = fileRow; }
    
    // Getters and setters for filePath
    const std::string& getFilePath() const { return m_filePath; }
    void setFilePath(const std::string& path) { m_filePath = path; }
    
    // Alias for getFilePath for compatibility
    const std::string& getPath() const { return m_filePath; }
    
    // Getters and setters for fileName
    std::string getFileName() const { return m_fileName.empty() ? extractFileName() : m_fileName; }
    void setFileName(const std::string& name) { m_fileName = name; }
    
    // Getters and setters for modifiedTime
    time_t getModifiedTime() const { return m_modifiedTime; }
    void setModifiedTime(time_t time) { m_modifiedTime = time; }
    
    // Getters and setters for fileSize
    int64_t getFileSize() const { return m_fileSize; }
    void setFileSize(int64_t size) { m_fileSize = size; }
    
    // Getters and setters for selected
    bool isSelected() const { return m_selected; }
    void setSelected(bool sel) { m_selected = sel; }

    bool isExists() const { return m_isExists; }
    void setExists(bool exists) { m_isExists = exists; }
    
private:
    int32_t m_id = -1;
    int32_t m_fileRow = -1;
    std::string m_filePath;
    std::string m_fileName;
    time_t m_modifiedTime = 0;
    int64_t m_fileSize = 0;
    bool m_selected = false;
    bool m_isExists = false;
    // Helper method to extract filename from path
    std::string extractFileName() const;
};

// Smart pointer type for FileData
using FileDataPtr = std::shared_ptr<FileData>;

} // namespace Core

#endif // FILEDATA_H 