#include "FileData.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <nlohmann/json.hpp>
#include "AppUtils.h"
#include "Logger.h"
#include "FileSystem.h"

namespace Core {

    bool FileData::init(int32_t id, int32_t fileRow, const std::string& path)
    {
        m_id = id;
        m_fileRow = fileRow;
        m_filePath = path;
        //get file name, modified time, file size
        m_fileName = extractFileName();
        m_modifiedTime = FileSystem::toTimeT(FileSystem::lastWriteTime(m_filePath));
        m_fileSize = std::filesystem::file_size(m_filePath);
        m_selected = true;
        m_isExists = std::filesystem::exists(m_filePath);
        return true;
    }

    bool FileData::saveToJson(json& j) const {
        j["id"] = m_id;
        j["fileRow"] = m_fileRow;
        j["name"] = m_fileName;
        j["path"] = m_filePath;
        j["modifiedTime"] = m_modifiedTime;
        j["fileSize"] = m_fileSize;
        j["selected"] = m_selected;
        return true;
    }

    bool FileData::loadFromJson(const json& j) {
        m_id = j.value("id", -1);
        if(m_id == -1) {
            return false;
        }
        m_fileRow = j.value("fileRow", -1);
        m_fileName = j.value("name", "");
        m_filePath = j.value("path", "");
        m_modifiedTime = j.value("modifiedTime", 0);
        m_fileSize = j.value("fileSize", 0);
        m_selected = j.value("selected", false);
        m_isExists = std::filesystem::exists(m_filePath);
        return true;
    }

    std::string FileData::extractFileName() const {
        if (m_filePath.empty()) {
            return "";
        }
        
        // Use std::filesystem to extract the filename
        std::filesystem::path path(m_filePath);
        return path.filename().string();
    }

} // namespace Core 
