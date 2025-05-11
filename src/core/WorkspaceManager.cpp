#include "WorkspaceManager.h"
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

// Use nlohmann/json for JSON handling
using json = nlohmann::json;

WorkspaceManager::WorkspaceManager() : activeWorkspaceId(-1) {
    // Constructor
}

WorkspaceManager::~WorkspaceManager() {
    // Destructor
}

bool WorkspaceManager::saveWorkspaces() {
    if(m_saveWorkspacePaused){
        m_hasPendingSaveWorkspace = true;
        return true;
    }
    // Get the workspaces file path from AppUtils
    std::string filePath = AppUtils::getWorkspacesFilePath();
    
    Logger::getInstance().info("WorkspaceManager Saving workspaces to: " + filePath);
    
    // Create parent directory if it doesn't exist
    std::filesystem::path path(filePath);
    if (!std::filesystem::exists(path.parent_path())) {
        std::filesystem::create_directories(path.parent_path());
        Logger::getInstance().info("WorkspaceManager Created directory: " + path.parent_path().string());
    }
        
    try {
        json rootObj;
        rootObj["formatVersion"] = 1;
        rootObj["configVersion"] = configVersion;
        rootObj["activeWorkspaceId"] = activeWorkspaceId;
        rootObj["nextWorkspaceId"] = nextWorkspaceId;

        json workspacesArray = json::array();        
        for (const auto& [id, workspace] : workspaces) {
            json workspaceObj;
            if(!workspace->saveToJson(workspaceObj)){
                Logger::getInstance().error("WorkspaceManager::saveWorkspaces Error saving workspace: " + workspace->getName());
                return false;
            }
            workspacesArray.push_back(workspaceObj);
        }
        rootObj["workspaces"] = workspacesArray;
        
        // Write to file
        std::ofstream file(filePath);
        if (!file.is_open()) {
            Logger::getInstance().info("WorkspaceManager Could not open workspace configuration file for writing");
            return false;
        }
        
        file << std::setw(4) << rootObj;
        file.close();
        
        Logger::getInstance().info("WorkspaceManager Successfully saved " + std::to_string(workspaces.size()) + " workspaces");
        return true;
    } catch (const std::exception& e) {
        Logger::getInstance().info("WorkspaceManager Error saving workspaces: " + std::string(e.what()));
        return false;
    }
}

bool WorkspaceManager::loadWorkspaces() {
    Logger::getInstance().info("Enter WorkspaceManager::loadWorkspaces");
    std::string filePath = AppUtils::getWorkspacesFilePath();
    
    Logger::getInstance().info("WorkspaceManager::loadWorkspaces Loading workspaces from: " + filePath);
    
    // Check if file exists
    if (!std::filesystem::exists(filePath)) {
        Logger::getInstance().info("WorkspaceManager::loadWorkspaces Workspaces file not found: " + filePath);
        return false;
    }
    
    // Check if file is valid JSON
    if (!isValidJsonFile(filePath)) {
        Logger::getInstance().info("WorkspaceManager::loadWorkspaces Invalid workspaces file: " + filePath);
        return false;
    }
    
    try {
        // Read JSON file
        std::ifstream file(filePath);
        json rootObj = json::parse(file);
        
        // Check config version
        
        if (rootObj.contains("configVersion")) {
            std::string version = rootObj["configVersion"];
            if(version != configVersion) {
                Logger::getInstance().error("WorkspaceManager::loadWorkspaces Incompatible config version: " + version + " (expected " + configVersion + ")");
                return false;
            }
        }
        
        //get "nextWorkspaceId" from rootObj
        nextWorkspaceId = rootObj.value("nextWorkspaceId", 1);

        if (rootObj.contains("workspaces") && rootObj["workspaces"].is_array()) {
            for (const auto& workspaceObj : rootObj["workspaces"]) {
                auto workspace = std::make_shared<WorkspaceData>();
                if(workspace->loadFromJson(workspaceObj)){
                    workspaces[workspace->getId()] = workspace;
                }else{
                    Logger::getInstance().error("WorkspaceManager::loadWorkspaces Error loading workspace");
                }
            }
        }
        
        
        // Set active workspace
        if (rootObj.contains("activeWorkspaceId")) {
            int64_t id = rootObj["activeWorkspaceId"];
            if (workspaces.find(id) != workspaces.end()) {
                setActiveWorkspace(id);
                Logger::getInstance().info("WorkspaceManager::loadWorkspaces Set active workspace to id " + std::to_string(id));
            } else {
                Logger::getInstance().error("WorkspaceManager::loadWorkspaces Invalid active workspace id: " + std::to_string(id));
            }
        }
        
        Logger::getInstance().info("WorkspaceManager::loadWorkspaces Successfully loaded " + std::to_string(workspaces.size()) + " workspaces");
        return true;
    } catch (const std::exception& e) {
        Logger::getInstance().error("WorkspaceManager::loadWorkspaces Error loading workspaces: " + std::string(e.what()));
        return false;
    }
}

void WorkspaceManager::beginWorkspaceUpdate() {
    m_saveWorkspacePaused = true;
}

void WorkspaceManager::commitWorkspaceUpdate() {
    m_saveWorkspacePaused = false;
    if(m_hasPendingSaveWorkspace){
        saveWorkspaces();
        m_hasPendingSaveWorkspace = false;
    }
}

void WorkspaceManager::rollbackWorkspaceUpdate() {
    m_saveWorkspacePaused = false;
    m_hasPendingSaveWorkspace = false;
}

int64_t WorkspaceManager::createWorkspace() {
    int64_t newId = nextWorkspaceId++;
    std::string name = "Workspace " + std::to_string(newId);    
    workspaces[newId] = std::make_shared<WorkspaceData>(newId,name);    
    setActiveWorkspace(newId);
    Logger::getInstance().info("WorkspaceManager Created workspace: " + name + " (id: " + std::to_string(newId) + ")");
    saveWorkspaces();
    return newId;
}

bool WorkspaceManager::removeWorkspace(int64_t id) {
    auto it = workspaces.find(id);
    if (it == workspaces.end()) {
        Logger::getInstance().info("WorkspaceManager Failed to remove workspace: Invalid id " + std::to_string(id));
        return false;
    }
    
    std::string name = it->second->getName();
    workspaces.erase(it);
    Logger::getInstance().info("WorkspaceManager Removed workspace: " + name);
    return true;
}

void WorkspaceManager::clearWorkspaces() {
    workspaces.clear();
    nextWorkspaceId = 1;
    activeWorkspaceId = -1;
    Logger::getInstance().info("WorkspaceManager Cleared all workspaces");
}

void WorkspaceManager::setActiveWorkspace(int64_t id) {
    if(activeWorkspaceId == id) {
        return;
    }
    //set previous active workspace to inactive
    if(activeWorkspaceId != -1){
        auto it = workspaces.find(activeWorkspaceId);
        if (it != workspaces.end()) {
            it->second->setActive(false);
        }
    }
    auto it = workspaces.find(id);
    if (it != workspaces.end()) {
        it->second->setActive(true);
        activeWorkspaceId = id;
        saveWorkspaces();
        Logger::getInstance().info("WorkspaceManager::setActiveWorkspace Set active workspace to id " + std::to_string(id) + 
                  " (" + it->second->getName() + ")");
    } else {
        Logger::getInstance().error("WorkspaceManager::setActiveWorkspace Failed to set active workspace: Invalid id " + std::to_string(id));
    }
}

WorkspaceDataPtr WorkspaceManager::getActiveWorkspaceData() {
    auto it = workspaces.find(activeWorkspaceId);
    if (it != workspaces.end()) {
        return it->second;
    }
    return nullptr;
}

WorkspaceDataPtr WorkspaceManager::getWorkspace(int64_t id) {
    auto it = workspaces.find(id);
    if (it != workspaces.end()) {
        return it->second;
    }
    return nullptr;
}

bool WorkspaceManager::isValidJsonFile(const std::string& filePath) {
    try {
        std::ifstream file(filePath);
        if(!file.is_open()) {
            return false;
        }
        [[maybe_unused]] auto ret= json::parse(file);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

std::vector<int64_t> WorkspaceManager::getAllWorkspaceIds() const {
    std::vector<int64_t> ids;
    std::multimap<int32_t, int64_t> sortedIds;
    for (const auto& [id, workspace] : workspaces) {
        sortedIds.insert({workspace->getSortIndex(), id});
    }
    for (const auto& [sortIndex, id] : sortedIds) {
        ids.push_back(id);
    }
    return ids;
}

int32_t WorkspaceManager::getWorkspaceSortIndex(int64_t id) const {
    auto it = workspaces.find(id);
    if (it != workspaces.end()) {
        return it->second->getSortIndex();
    }
    return -1;
}

void WorkspaceManager::setWorkspaceSortIndex(int64_t id, int32_t sortIndex) {
    auto it = workspaces.find(id);
    if (it != workspaces.end()) {
        it->second->setSortIndex(sortIndex);
        saveWorkspaces();
    }
}

bool WorkspaceManager::setWorkspaceName(int64_t id, const std::string& name) {
    if(name.empty()) {
        Logger::getInstance().warning("WorkspaceManager Failed to set workspace name: Empty name");
        return false;
    }
    auto it = workspaces.find(id);
    if (it == workspaces.end()) {
        Logger::getInstance().info("WorkspaceManager Failed to set workspace name: Invalid id " + std::to_string(id));
        return false;
    }
    std::string oldName = it->second->getName();
    it->second->setName(name);
    Logger::getInstance().info("WorkspaceManager Successfully renamed workspace " + std::to_string(id) + " from '" + oldName + "' to '" + name + "'");
    saveWorkspaces();
    return true;
}

bool WorkspaceManager::isValidWorkspaceName(const std::string& name) const {
    // Name cannot be empty
    if (name.empty()) {
        return false;
    }

    // Name cannot be too long (arbitrary limit of 100 characters)
    if (name.length() > 100) {
        return false;
    }

    // Name cannot contain only whitespace
    if (std::all_of(name.begin(), name.end(), ::isspace)) {
        return false;
    }

    // Name cannot contain certain special characters
    const std::string invalidChars = "\\/:*?\"<>|";
    if (name.find_first_of(invalidChars) != std::string::npos) {
        return false;
    }

    return true;
}

////////////////////////////////////////////////////////////
// File management
////////////////////////////////////////////////////////////

int32_t WorkspaceManager::addFileToWorkspace(int64_t workspaceId, int32_t fileRow, const std::string& filePath) {
    auto it = workspaces.find(workspaceId);
    if (it == workspaces.end()) {
        Logger::getInstance().info("WorkspaceManager Failed to add file to workspace: Invalid id " + std::to_string(workspaceId));
        return false;
    }
    Logger::getInstance().info("WorkspaceManager::addFileToWorkspace Adding file to workspace " + std::to_string(workspaceId) + ": " + filePath);
    auto fileId = it->second->addFile(fileRow,filePath);
    if(0 < fileId){
        saveWorkspaces();
    }else{
        Logger::getInstance().error("WorkspaceManager::addFileToWorkspace Failed to add file to workspace: " + filePath);
    }
    return fileId;
}

bool WorkspaceManager::removeFileFromWorkspace(int64_t workspaceId, int32_t fileId) {
    auto it = workspaces.find(workspaceId);
    if (it == workspaces.end()) {
        Logger::getInstance().info("WorkspaceManager Failed to remove file from workspace: Invalid id " + std::to_string(workspaceId));
        return false;
    }
    Logger::getInstance().info("WorkspaceManager::removeFileFromWorkspace Removing file from workspace " + std::to_string(workspaceId) + ": " + std::to_string(fileId));
    it->second->removeFile(fileId);
    saveWorkspaces();
    return true;
}

FileDataPtr WorkspaceManager::getFileData(int64_t workspaceId, int32_t fileId) {
    auto it = workspaces.find(workspaceId);
    if (it == workspaces.end()) {
        Logger::getInstance().info("WorkspaceManager Failed to get file data: Invalid workspace id " + std::to_string(workspaceId));
        return nullptr;
    }
    return it->second->getFileData(fileId);
}

void WorkspaceManager::updateFileRow(int64_t workspaceId, int32_t fileId, int32_t fileRow) {
    auto it = workspaces.find(workspaceId);
    if (it == workspaces.end()) {
        Logger::getInstance().info("WorkspaceManager Failed to update file row: Invalid workspace id " + std::to_string(workspaceId));
        return;
    }
    it->second->updateFileRow(fileId, fileRow);
    saveWorkspaces();
}

void WorkspaceManager::updateFileSelection(int64_t workspaceId, int32_t fileId, bool selected)
{
    auto it = workspaces.find(workspaceId);
    if (it == workspaces.end()) {
        Logger::getInstance().info("WorkspaceManager Failed to update file selection: Invalid workspace id " + std::to_string(workspaceId));
        return;
    }
    it->second->updateFileSelection(fileId, selected);
    saveWorkspaces();
}

std::vector<FileDataPtr> WorkspaceManager::getFileDataList(int64_t workspaceId) {
    auto it = workspaces.find(workspaceId);
    if (it == workspaces.end()) {
        Logger::getInstance().info("WorkspaceManager Failed to get file data list: Invalid workspace id " + std::to_string(workspaceId));
        return std::vector<FileDataPtr>();
    }   
    return it->second->getFileDataList();
}

void WorkspaceManager::beginFileUpdate(int64_t workspaceId) {
    auto it = workspaces.find(workspaceId);
    if (it == workspaces.end()) {
        Logger::getInstance().info("WorkspaceManager Failed to begin file update: Invalid workspace id " + std::to_string(workspaceId));
        return;
    }
    it->second->beginFileUpdate();
}

void WorkspaceManager::commitFileUpdate(int64_t workspaceId) {
    auto it = workspaces.find(workspaceId);
    if (it == workspaces.end()) {
        Logger::getInstance().info("WorkspaceManager Failed to commit file update: Invalid workspace id " + std::to_string(workspaceId));
        return;
    }
    it->second->commitFileUpdate();
}

void WorkspaceManager::rollbackFileUpdate(int64_t workspaceId) {
    auto it = workspaces.find(workspaceId);
    if (it == workspaces.end()) {
        Logger::getInstance().info("WorkspaceManager Failed to rollback file update: Invalid workspace id " + std::to_string(workspaceId));
        return;
    } 
    it->second->rollbackFileUpdate();
}

void WorkspaceManager::reloadFilesInWorkspace(int64_t workspaceId) {
    auto it = workspaces.find(workspaceId);
    if (it == workspaces.end()) {
        Logger::getInstance().info("WorkspaceManager Failed to reload files in workspace: Invalid workspace id " + std::to_string(workspaceId));
        return;
    }
    it->second->reloadFiles();
}
////////////////////////////////////////////////////////////
// Filter management
////////////////////////////////////////////////////////////

int32_t WorkspaceManager::addFilterToWorkspace(int64_t workspaceId, const FilterData& filter) {
    auto it = workspaces.find(workspaceId);
    if (it == workspaces.end()) {
        Logger::getInstance().info("WorkspaceManager Failed to add filter to workspace: Invalid workspace id " + std::to_string(workspaceId));
        return -1;
    }
    int32_t filterId = it->second->addFilter(filter);
    if(filterId >= 0){
        saveWorkspaces();
    }else{
        Logger::getInstance().error("WorkspaceManager::addFilterToWorkspace Failed to add filter to workspace: " + filter.getPattern());
    }
    return filterId;
}

bool WorkspaceManager::removeFilterFromWorkspace(int64_t workspaceId, int32_t filterId) {
    auto it = workspaces.find(workspaceId);
    if (it == workspaces.end()) {
        Logger::getInstance().info("WorkspaceManager Failed to remove filter from workspace: Invalid workspace id " + std::to_string(workspaceId));
        return false;
    }
    it->second->removeFilter(filterId);
    saveWorkspaces();
    return true;
}

std::vector<FilterDataPtr> WorkspaceManager::getFilterDataList(int64_t workspaceId) {
    auto it = workspaces.find(workspaceId);
    if (it == workspaces.end()) {
        Logger::getInstance().info("WorkspaceManager Failed to get filter data list: Invalid workspace id " + std::to_string(workspaceId));
        return std::vector<FilterDataPtr>();
    }
    return it->second->getFilterDataList();
}

void WorkspaceManager::updateFilterRows(int64_t workspaceId, std::list<int32_t> filterIds) {
    auto it = workspaces.find(workspaceId);
    if (it == workspaces.end()) {
        Logger::getInstance().info("WorkspaceManager Failed to update filter rows: Invalid workspace id " + std::to_string(workspaceId));
        return;
    }
    it->second->updateFilterRows(filterIds);
    saveWorkspaces();
}

void WorkspaceManager::updateFilter(int64_t workspaceId, const FilterData& filter) {
    auto it = workspaces.find(workspaceId);
    if (it == workspaces.end()) {
        Logger::getInstance().info("WorkspaceManager Failed to update filter: Invalid workspace id " + std::to_string(workspaceId));
        return;
    }
    it->second->updateFilter(filter);
    saveWorkspaces();
}

void WorkspaceManager::beginFilterUpdate(int64_t workspaceId) {
    auto it = workspaces.find(workspaceId);
    if (it == workspaces.end()) {
        Logger::getInstance().info("WorkspaceManager Failed to begin filter update: Invalid workspace id " + std::to_string(workspaceId));
        return;
    }
    it->second->beginFilterUpdate();
}

void WorkspaceManager::commitFilterUpdate(int64_t workspaceId) {
    auto it = workspaces.find(workspaceId);
    if (it == workspaces.end()) {
        Logger::getInstance().info("WorkspaceManager Failed to commit filter update: Invalid workspace id " + std::to_string(workspaceId));
        return;
    }
    it->second->commitFilterUpdate();
}

void WorkspaceManager::rollbackFilterUpdate(int64_t workspaceId) {
    auto it = workspaces.find(workspaceId);
    if (it == workspaces.end()) {
        Logger::getInstance().info("WorkspaceManager Failed to rollback filter update: Invalid workspace id " + std::to_string(workspaceId));
        return;
    }
    it->second->rollbackFilterUpdate();
}

std::map<int32_t, int32_t> WorkspaceManager::getFilterMatchCounts(int64_t workspaceId) {
    auto it = workspaces.find(workspaceId);
    if (it == workspaces.end()) {
        Logger::getInstance().info("WorkspaceManager Failed to get filter match counts: Invalid workspace id " + std::to_string(workspaceId));
        return std::map<int32_t, int32_t>();
    }
    return it->second->getFilterMatchCounts();
}

std::string WorkspaceManager::getNextFilterColor(int64_t workspaceId) {
    auto it = workspaces.find(workspaceId);
    if (it == workspaces.end()) {
        Logger::getInstance().info("WorkspaceManager Failed to get next filter color: Invalid workspace id " + std::to_string(workspaceId));
        return "#000000";
    }
    return it->second->getNextFilterColor();
}

////////////////////////////////////////////////////////////
// Search management
////////////////////////////////////////////////////////////

int32_t WorkspaceManager::addSearchToWorkspace(int64_t workspaceId, const SearchData& search) {
    auto it = workspaces.find(workspaceId);
    if (it == workspaces.end()) {
        Logger::getInstance().info("WorkspaceManager Failed to add search to workspace: Invalid workspace id " + std::to_string(workspaceId));
        return -1;
    }
    int32_t searchId = it->second->addSearch(search);
    if(searchId >= 0){
        saveWorkspaces();
    }else{
        Logger::getInstance().error("WorkspaceManager::addSearchToWorkspace Failed to add search to workspace: " + search.getPattern());
    }
    return searchId;
}

bool WorkspaceManager::removeSearchFromWorkspace(int64_t workspaceId, int32_t searchId) {
    auto it = workspaces.find(workspaceId);
    if (it == workspaces.end()) {
        Logger::getInstance().info("WorkspaceManager Failed to remove search from workspace: Invalid workspace id " + std::to_string(workspaceId));
        return false;
    }
    it->second->removeSearch(searchId);
    saveWorkspaces();
    return true;
}

std::vector<SearchDataPtr> WorkspaceManager::getSearchDataList(int64_t workspaceId) {
    auto it = workspaces.find(workspaceId);
    if (it == workspaces.end()) {
        Logger::getInstance().info("WorkspaceManager Failed to get search data list: Invalid workspace id " + std::to_string(workspaceId));
        return std::vector<SearchDataPtr>();
    }
    return it->second->getSearchDataList();
}

void WorkspaceManager::updateSearchRows(int64_t workspaceId, std::list<int32_t> searchIds) {
    auto it = workspaces.find(workspaceId);
    if (it == workspaces.end()) {
        Logger::getInstance().info("WorkspaceManager Failed to update search rows: Invalid workspace id " + std::to_string(workspaceId));
        return;
    }
    it->second->updateSearchRows(searchIds);
    saveWorkspaces();
}

void WorkspaceManager::updateSearch(int64_t workspaceId, const SearchData& search) {
    auto it = workspaces.find(workspaceId);
    if (it == workspaces.end()) {
        Logger::getInstance().info("WorkspaceManager Failed to update search: Invalid workspace id " + std::to_string(workspaceId));
        return;
    }
    it->second->updateSearch(search);
    saveWorkspaces();
}

void WorkspaceManager::beginSearchUpdate(int64_t workspaceId) {
    auto it = workspaces.find(workspaceId);
    if (it == workspaces.end()) {
        Logger::getInstance().info("WorkspaceManager Failed to begin search update: Invalid workspace id " + std::to_string(workspaceId));
        return;
    }
    it->second->beginSearchUpdate();
}

void WorkspaceManager::commitSearchUpdate(int64_t workspaceId) {
    auto it = workspaces.find(workspaceId);
    if (it == workspaces.end()) {
        Logger::getInstance().info("WorkspaceManager Failed to commit search update: Invalid workspace id " + std::to_string(workspaceId));
        return;
    }
    it->second->commitSearchUpdate();
}

void WorkspaceManager::rollbackSearchUpdate(int64_t workspaceId) {
    auto it = workspaces.find(workspaceId);
    if (it == workspaces.end()) {
        Logger::getInstance().info("WorkspaceManager Failed to rollback search update: Invalid workspace id " + std::to_string(workspaceId));
        return;
    }
    it->second->rollbackSearchUpdate();
}

std::map<int32_t, int32_t> WorkspaceManager::getSearchMatchCounts(int64_t workspaceId) {
    auto it = workspaces.find(workspaceId);
    if (it == workspaces.end()) {
        Logger::getInstance().info("WorkspaceManager Failed to get search match counts: Invalid workspace id " + std::to_string(workspaceId));
        return std::map<int32_t, int32_t>();
    }
    return it->second->getSearchMatchCounts();
}


std::string WorkspaceManager::getNextSearchColor(int64_t workspaceId) {
    auto it = workspaces.find(workspaceId);
    if (it == workspaces.end()) {
        Logger::getInstance().info("WorkspaceManager Failed to get next filter color: Invalid workspace id " + std::to_string(workspaceId));
        return "#000000";
    }
    return it->second->getNextSearchColor();
}

////////////////////////////////////////////////////////////
// Output management
////////////////////////////////////////////////////////////

std::vector<std::shared_ptr<OutputLine>> WorkspaceManager::getOutputStringList(int64_t workspaceId) {
    auto it = workspaces.find(workspaceId);
    if (it == workspaces.end()) {
        Logger::getInstance().info("WorkspaceManager Failed to get output string list: Invalid workspace id " + std::to_string(workspaceId));
        return std::vector<std::shared_ptr<OutputLine>>();
    }
    return it->second->getOutputStringList();
}

bool WorkspaceManager::getNextMatchByFilter(int64_t workspaceId, int32_t filterId, int32_t lineIndex, int32_t charIndex,
                              int32_t& matchLineIndex, int32_t& matchCharStartIndex, int32_t& matchCharEndIndex) {
    auto it = workspaces.find(workspaceId);
    if (it == workspaces.end()) {
        Logger::getInstance().info("WorkspaceManager Failed to get next match by filter: Invalid workspace id " + std::to_string(workspaceId)); 
        return false;
    }
    return it->second->getNextMatchByFilter(filterId, lineIndex, charIndex, matchLineIndex, matchCharStartIndex, matchCharEndIndex);
}

bool WorkspaceManager::getPreviousMatchByFilter(int64_t workspaceId, int32_t filterId, int32_t lineIndex, int32_t charIndex,
                              int32_t& matchLineIndex, int32_t& matchCharStartIndex, int32_t& matchCharEndIndex) {
    auto it = workspaces.find(workspaceId);
    if (it == workspaces.end()) {
        Logger::getInstance().info("WorkspaceManager Failed to get previous match by filter: Invalid workspace id " + std::to_string(workspaceId));
        return false;
    }   
    return it->second->getPreviousMatchByFilter(filterId, lineIndex, charIndex, matchLineIndex, matchCharStartIndex, matchCharEndIndex);
}

bool WorkspaceManager::getNextMatchBySearch(int64_t workspaceId, int32_t searchId, int32_t lineIndex, int32_t charIndex,
                              int32_t& matchLineIndex, int32_t& matchCharStartIndex, int32_t& matchCharEndIndex) {
    auto it = workspaces.find(workspaceId);
    if (it == workspaces.end()) {
        Logger::getInstance().info("WorkspaceManager Failed to get next match by search: Invalid workspace id " + std::to_string(workspaceId));
        return false;
    }
    return it->second->getNextMatchBySearch(searchId, lineIndex, charIndex, matchLineIndex, matchCharStartIndex, matchCharEndIndex);
}

bool WorkspaceManager::getPreviousMatchBySearch(int64_t workspaceId, int32_t searchId, int32_t lineIndex, int32_t charIndex,
                              int32_t& matchLineIndex, int32_t& matchCharStartIndex, int32_t& matchCharEndIndex) {
    auto it = workspaces.find(workspaceId);
    if (it == workspaces.end()) {
        Logger::getInstance().info("WorkspaceManager Failed to get previous match by search: Invalid workspace id " + std::to_string(workspaceId));
        return false;
    }
    return it->second->getPreviousMatchBySearch(searchId, lineIndex, charIndex, matchLineIndex, matchCharStartIndex, matchCharEndIndex);
}

} // namespace Core
