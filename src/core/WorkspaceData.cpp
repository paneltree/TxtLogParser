#include "WorkspaceData.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <nlohmann/json.hpp>
#include "AppUtils.h"
#include "Logger.h"

namespace Core {

bool WorkspaceData::saveToJson(json& j) const {
    j["id"] = id;
    j["name"] = workspaceName;
    j["sortIndex"] = sortIndex;

    j["nextFileId"] = m_nextFileId;
    json filesArray = json::array();
    for (const auto& [fileId, file] : m_files) {
        json fileJson;
        if(!file->saveToJson(fileJson)){
            Logger::getInstance().error("WorkspaceData::saveToJson Error saving file: " + file->getFilePath());
            return false;
        }
        filesArray.push_back(fileJson);
    }
    j["files"] = filesArray;
    
    // Save m_filters
    j["nextFilterId"] = m_nextFilterId;
    json filtersArray = json::array();
    for (const auto& [filterId, filter] : m_filters) {
        json filterJson;
        filter->saveToJson(filterJson);
        filtersArray.push_back(filterJson);
    }
    j["filters"] = filtersArray;
    
    // Save m_searches
    j["nextSearchId"] = m_nextSearchId;
    json searchesArray = json::array();
    for (const auto& [searchId, search] : m_searches) {
        json searchJson;
        search->saveToJson(searchJson);
        searchesArray.push_back(searchJson);
    }
    j["searches"] = searchesArray;
    
    return true;
}
bool WorkspaceData::loadFromJson(const json& j) {
    id = j.value("id", -1);
    workspaceName = j.value("name", "");
    sortIndex = j.value("sortIndex", -1);
 
    // Load files
    m_nextFileId = j.value("nextFileId", m_nextFileId);  
    if (j.contains("files") && j["files"].is_array()) {
        for (const auto& fileJson : j["files"]) {
            auto fileData = std::make_shared<FileData>();
            if (fileData->loadFromJson(fileJson)) {
                addFile(fileData);
            }else{
                Logger::getInstance().error("WorkspaceData::loadFromJson Error loading file: " + fileData->getFilePath());
            }
        }
    }
    
    // Load m_filters
    m_nextFilterId = j.value("nextFilterId", m_nextFilterId);
    if (j.contains("filters") && j["filters"].is_array()) {
        for (const auto& filterJson : j["filters"]) {
            FilterData filterData;
            if (filterData.loadFromJson(filterJson)) {
                addFilter(filterData);
            }
        }
    }
    
    // Load m_searches
    m_nextSearchId = j.value("nextSearchId", m_nextSearchId);
    if (j.contains("searches") && j["searches"].is_array()) {
        for (const auto& searchJson : j["searches"]) {
            auto searchData = std::make_shared<SearchData>();
            if (searchData->loadFromJson(searchJson)) {
                addSearch(*searchData);
            }
        }
    }
    
    return true;
}

void WorkspaceData::setActive(bool bActive) {
    m_bActive = bActive;
    m_outputData.setActive(bActive);
}
////////////////////////////////////////////////////////////
// File management
////////////////////////////////////////////////////////////

int32_t WorkspaceData::addFile(int32_t fileRow, const std::string& filePath)
{
    auto fileData = std::make_shared<FileData>();
    if(fileData->init(m_nextFileId, fileRow, filePath)){
        int32_t fileId = m_nextFileId;
        ++m_nextFileId;
        addFile(fileData);
        return fileId;
    }
    return -1;
}

void WorkspaceData::addFile(FileDataPtr file) {
    if (file && file->getFileId() >= 0) {
        m_files[file->getFileId()] = file;
        if(file->isSelected()){
            m_outputData.addFile(file);
        }
    }
}

void WorkspaceData::removeFile(int32_t id) {
    auto it = m_files.find(id);
    if (it != m_files.end()) {
        m_outputData.removeFile(id);
        m_files.erase(it);
    }
}

FileDataPtr WorkspaceData::getFileData(int32_t id) const {
    auto it = m_files.find(id);
    if (it != m_files.end()) {
        return it->second;
    }
    return nullptr;
}

void WorkspaceData::updateFileRow(int32_t id, int32_t fileRow) {
    auto it = m_files.find(id);
    if (it != m_files.end()) {
        it->second->setFileRow(fileRow);
        m_outputData.updateFileRow(id, fileRow);
    }
}

void WorkspaceData::updateFileSelection(int32_t id, bool selected) {
    auto it = m_files.find(id);
    if (it != m_files.end()) {
        it->second->setSelected(selected);
        if(selected){
            m_outputData.addFile(it->second);
        }else{
            m_outputData.removeFile(id);
        }
    }
}

std::vector<FileDataPtr> WorkspaceData::getFileDataList() const {
    std::vector<FileDataPtr> result;
    //sort the files by fileRow
    std::map<int32_t/*fileRow*/, FileDataPtr> fileRows;
    for(const auto& [id, file] : m_files){
        fileRows[file->getFileRow()] = file;
    }
    result.reserve(fileRows.size());    
    for(const auto& [row, file] : fileRows){    
        result.push_back(file);
    }
    return result;
}

void WorkspaceData::beginFileUpdate() {
    m_bInFileUpdateTransaction = true;
    m_outputData.pauseRefresh();
}

void WorkspaceData::commitFileUpdate() {
    m_bInFileUpdateTransaction = false;
    m_outputData.resumeRefresh();
    m_outputData.refresh();
}

void WorkspaceData::rollbackFileUpdate() {
    m_bInFileUpdateTransaction = false;
    m_outputData.resumeRefresh();
}

void WorkspaceData::reloadFiles() {
    m_outputData.reloadFiles();
}

////////////////////////////////////////////////////////////
// Filter management
////////////////////////////////////////////////////////////

int32_t WorkspaceData::addFilter(const FilterData& filterData) {
    auto filter = std::make_shared<FilterData>(filterData);
    if(filter->getId() == -1){
        filter->setId(m_nextFilterId++);
    }
    m_filters[filter->getId()] = filter;
    m_outputData.addFilter(filter);
    m_filterSearchColorManager.popColor(filter->getColor());
    return filter->getId();
}

void WorkspaceData::removeFilter(int32_t filterId) {
    auto it = m_filters.find(filterId);
    if (it != m_filters.end()) {
        m_filterSearchColorManager.pushColor(it->second->getColor());
        m_outputData.removeFilter(filterId);
        m_filters.erase(it);
    }
}

std::vector<FilterDataPtr> WorkspaceData::getFilterDataList() {
    std::vector<FilterDataPtr> result;
    std::map<int32_t/*row*/, int32_t/*filterId*/> filterRows;
    for(const auto& [id, filter] : m_filters){
        filterRows[filter->getRow()] = id;
    }
    result.reserve(filterRows.size());
    for(const auto& [row, id] : filterRows){
        result.push_back(m_filters[id]);
    }
    return result;
}

std::map<int32_t, int32_t> WorkspaceData::getFilterMatchCounts() const {
    return m_outputData.getFilterMatchCounts();
}

void WorkspaceData::updateFilterRow(int32_t filterId, int32_t filterRow) {
    auto it = m_filters.find(filterId);
    if (it != m_filters.end()) {
        it->second->setRow(filterRow);
        m_outputData.updateFilterRow(filterId, filterRow);
    }
}

void WorkspaceData::updateFilter(const FilterData& filter) {
    auto it = m_filters.find(filter.getId());
    if (it != m_filters.end()) {
        bool changed = false;
        std::string oldColor = it->second->getColor();
        it->second->update(filter, &changed);
        if(changed){            
            std::string newColor = it->second->getColor();
            if(oldColor != newColor){
                m_filterSearchColorManager.pushColor(oldColor);
                m_filterSearchColorManager.popColor(newColor);
            }
            m_outputData.updateFilter(filter);
        }
    }
}

void WorkspaceData::beginFilterUpdate() {
    m_bInFilterUpdateTransaction = true;
    m_outputData.pauseRefresh();
}

void WorkspaceData::commitFilterUpdate() {
    m_bInFilterUpdateTransaction = false;
    m_outputData.resumeRefresh();
    m_outputData.refresh();
}

void WorkspaceData::rollbackFilterUpdate() {
    m_bInFilterUpdateTransaction = false;
    m_outputData.resumeRefresh();
}

std::string WorkspaceData::getNextFilterColor() {
    return m_filterSearchColorManager.getNextColor();
}

////////////////////////////////////////////////////////////
// Search management
////////////////////////////////////////////////////////////

int32_t WorkspaceData::addSearch(const SearchData& search) {
    auto searchData = std::make_shared<SearchData>(search);
    if(searchData->getId() == -1){
        searchData->setId(m_nextSearchId++);
    }
    m_searches[searchData->getId()] = searchData;
    m_outputData.addSearch(searchData);
    m_filterSearchColorManager.popColor(searchData->getColor());
    return searchData->getId();
}

void WorkspaceData::removeSearch(int32_t searchId) {
    auto it = m_searches.find(searchId);
    if (it != m_searches.end()) {
        m_filterSearchColorManager.pushColor(it->second->getColor());
        m_outputData.removeSearch(searchId);
        m_searches.erase(it);
    }
}

std::vector<SearchDataPtr> WorkspaceData::getSearchDataList() {
    std::vector<SearchDataPtr> result;
    std::map<int32_t/*row*/, int32_t/*searchId*/> searchRows;
    for(const auto& [id, search] : m_searches){
        searchRows[search->getRow()] = id;
    }
    result.reserve(searchRows.size());
    for(const auto& [row, id] : searchRows){
        result.push_back(m_searches[id]);
    }
    return result;
}

std::map<int32_t, int32_t> WorkspaceData::getSearchMatchCounts() const {
    return m_outputData.getSearchMatchCounts();
}

void WorkspaceData::updateSearchRow(int32_t searchId, int32_t searchRow) {
    auto it = m_searches.find(searchId);
    if (it != m_searches.end()) {
        it->second->setRow(searchRow);
        m_outputData.updateSearchRow(searchId, searchRow);
    }
}

void WorkspaceData::updateSearch(const SearchData& search) {
    auto it = m_searches.find(search.getId());
    if (it != m_searches.end()) {
        std::string oldColor = it->second->getColor();
        it->second->update(search);
        std::string newColor = it->second->getColor();
        if(oldColor != newColor){
            m_filterSearchColorManager.pushColor(oldColor);
            m_filterSearchColorManager.popColor(newColor);
        }
        m_outputData.updateSearch(search);
    }
}

void WorkspaceData::beginSearchUpdate() {
    m_bInSearchUpdateTransaction = true;
    m_outputData.pauseRefresh();
}

void WorkspaceData::commitSearchUpdate() {
    m_bInSearchUpdateTransaction = false;
    m_outputData.resumeRefresh();
    m_outputData.refresh();
}

void WorkspaceData::rollbackSearchUpdate() {
    m_bInSearchUpdateTransaction = false;
    m_outputData.resumeRefresh();
}

std::string WorkspaceData::getNextSearchColor() {
    return m_filterSearchColorManager.getNextColor();
}

//////////////////////////////////////////////////////////////////
///     Output management
////////////////////////////////////////////////////////////////

std::vector<std::shared_ptr<OutputLine>> WorkspaceData::getOutputStringList() const {
    return m_outputData.getOutputStringList();
}

bool WorkspaceData::getNextMatchByFilter(int32_t filterId, int32_t lineIndex, int32_t charIndex,
                              int32_t& matchLineIndex, int32_t& matchCharStartIndex, int32_t& matchCharEndIndex) {
    return m_outputData.getNextMatchByFilter(filterId, lineIndex, charIndex, matchLineIndex, matchCharStartIndex, matchCharEndIndex);
}

bool WorkspaceData::getPreviousMatchByFilter(int32_t filterId, int32_t lineIndex, int32_t charIndex,
                              int32_t& matchLineIndex, int32_t& matchCharStartIndex, int32_t& matchCharEndIndex) {
    return m_outputData.getPreviousMatchByFilter(filterId, lineIndex, charIndex, matchLineIndex, matchCharStartIndex, matchCharEndIndex);
}

bool WorkspaceData::getNextMatchBySearch(int32_t searchId, int32_t lineIndex, int32_t charIndex,
                              int32_t& matchLineIndex, int32_t& matchCharStartIndex, int32_t& matchCharEndIndex) {
    return m_outputData.getNextMatchBySearch(searchId, lineIndex, charIndex, matchLineIndex, matchCharStartIndex, matchCharEndIndex);
}

bool WorkspaceData::getPreviousMatchBySearch(int32_t searchId, int32_t lineIndex, int32_t charIndex,
                              int32_t& matchLineIndex, int32_t& matchCharStartIndex, int32_t& matchCharEndIndex) {
    return m_outputData.getPreviousMatchBySearch(searchId, lineIndex, charIndex, matchLineIndex, matchCharStartIndex, matchCharEndIndex);
}

} // namespace Core
