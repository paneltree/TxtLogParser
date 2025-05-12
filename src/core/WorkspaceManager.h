#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <functional>
#include "WorkspaceData.h"
#include "AppUtils.h"

namespace Core {

/**
 * @brief Pure C++ class for managing workspaces
 * 
 * This class replaces the Qt-dependent WorkspaceManager with a pure C++ implementation.
 */
class WorkspaceManager {
public:
    using LogCallback = std::function<void(const std::string&)>;
    
    WorkspaceManager();
    ~WorkspaceManager();


    // Configuration
    bool loadWorkspaces();
    bool saveWorkspaces();
    void beginWorkspaceUpdate();
    void commitWorkspaceUpdate();
    void rollbackWorkspaceUpdate();
    
    // Workspace management
    int64_t createWorkspace();
    bool removeWorkspace(int64_t id);
    void clearWorkspaces();
    void setActiveWorkspace(int64_t id);
    
    // Getters
    WorkspaceDataPtr getActiveWorkspaceData();
    WorkspaceDataPtr getWorkspace(int64_t id);
    std::vector<int64_t> getAllWorkspaceIds() const;
    size_t getActiveWorkspace() const { return activeWorkspaceId; }
    int32_t getWorkspaceSortIndex(int64_t id) const;
    void setWorkspaceSortIndex(int64_t id, int32_t sortIndex);
    
    // Workspace name management
    bool setWorkspaceName(int64_t id, const std::string& name);
    bool isValidWorkspaceName(const std::string& name) const;

    // File management
    int32_t addFileToWorkspace(int64_t workspaceId, int32_t fileRow, const std::string& filePath);
    bool removeFileFromWorkspace(int64_t workspaceId, int32_t fileId);
    FileDataPtr getFileData(int64_t workspaceId, int32_t fileId);
    void updateFileRow(int64_t workspaceId, int32_t fileId, int32_t fileRow);
    void updateFileSelection(int64_t workspaceId, int32_t fileId, bool selected);
    std::vector<FileDataPtr> getFileDataList(int64_t workspaceId);
    void beginFileUpdate(int64_t workspaceId);
    void commitFileUpdate(int64_t workspaceId);
    void rollbackFileUpdate(int64_t workspaceId);
    void reloadFilesInWorkspace(int64_t workspaceId);

    // Filter management
    int32_t addFilterToWorkspace(int64_t workspaceId, const FilterData& filter);
    bool removeFilterFromWorkspace(int64_t workspaceId, int32_t filterId);
    std::vector<FilterDataPtr> getFilterDataList(int64_t workspaceId);
    void updateFilterRows(int64_t workspaceId, std::list<int32_t> filterIds);
    void updateFilter(int64_t workspaceId, const FilterData& filter);
    void beginFilterUpdate(int64_t workspaceId);
    void commitFilterUpdate(int64_t workspaceId);
    void rollbackFilterUpdate(int64_t workspaceId);
    std::map<int32_t, int32_t> getFilterMatchCounts(int64_t workspaceId);
    std::string getNextFilterColor(int64_t workspaceId);

    // Search management
    int32_t addSearchToWorkspace(int64_t workspaceId, const SearchData& search);
    bool removeSearchFromWorkspace(int64_t workspaceId, int32_t searchId);
    std::vector<SearchDataPtr> getSearchDataList(int64_t workspaceId);
    void updateSearchRows(int64_t workspaceId, std::list<int32_t> searchIds);
    void updateSearch(int64_t workspaceId, const SearchData& search);
    void beginSearchUpdate(int64_t workspaceId);
    void commitSearchUpdate(int64_t workspaceId);
    void rollbackSearchUpdate(int64_t workspaceId);
    std::map<int32_t, int32_t> getSearchMatchCounts(int64_t workspaceId);
    std::string getNextSearchColor(int64_t workspaceId);

    /// Output management
    std::vector<std::shared_ptr<OutputLine>> getOutputStringList(int64_t workspaceId);
    bool getNextMatchByFilter(int64_t workspaceId, int32_t filterId, int32_t lineIndex, int32_t charIndex,
                              int32_t& matchLineIndex, int32_t& matchCharStartIndex, int32_t& matchCharEndIndex);
    bool getPreviousMatchByFilter(int64_t workspaceId, int32_t filterId, int32_t lineIndex, int32_t charIndex,
                                  int32_t& matchLineIndex, int32_t& matchCharStartIndex, int32_t& matchCharEndIndex);
    bool getNextMatchBySearch(int64_t workspaceId, int32_t searchId, int32_t lineIndex, int32_t charIndex,
                              int32_t& matchLineIndex, int32_t& matchCharStartIndex, int32_t& matchCharEndIndex);
    bool getPreviousMatchBySearch(int64_t workspaceId, int32_t searchId, int32_t lineIndex, int32_t charIndex,
                                  int32_t& matchLineIndex, int32_t& matchCharStartIndex, int32_t& matchCharEndIndex);
private:
    std::map<int64_t, WorkspaceDataPtr> workspaces;
    std::string configVersion = "1.0";
    int64_t nextWorkspaceId = 1;
    int64_t activeWorkspaceId = -1;
    LogCallback logCallback;
    bool m_saveWorkspacePaused = false;
    bool m_hasPendingSaveWorkspace = false;
    // Helper methods
    bool isValidJsonFile(const std::string& filePath);

};

} // namespace Core
