#ifndef CORE_WORKSPACEDATA_H
#define CORE_WORKSPACEDATA_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <algorithm>
#include "FileData.h"
#include "FilterData.h"
#include "SearchData.h"
#include "OutputData.h"
#include "FilterSearchColorManager.h"
namespace Core {

/**
 * @brief Pure C++ class representing workspace data
 * 
 * This class replaces the Qt-dependent WorkspaceData class with a pure C++ implementation.
 */
class WorkspaceData {
public:
    WorkspaceData() = default;
    WorkspaceData(int64_t id, const std::string& name) : id(id), workspaceName(name) {}

    bool saveToJson(json& j) const;
    bool loadFromJson(const json& j);

    // Getters and setters
    int64_t getId() const { return id; }
    const std::string& getName() const { return workspaceName; }
    void setName(const std::string& name) { workspaceName = name; }
    int32_t getSortIndex() const { return sortIndex; }
    void setSortIndex(int32_t index) { sortIndex = index; }
    bool isActive() const { return m_bActive; }
    void setActive(bool bActive);
    
    // File management
    int32_t addFile(int32_t fileRow, const std::string& filePath);
    void addFile(FileDataPtr file);
    void removeFile(int32_t id);
    FileDataPtr getFileData(int32_t id) const;
    void updateFileRow(int32_t id, int32_t fileRow);
    void updateFileSelection(int32_t id, bool selected);
    std::vector<FileDataPtr> getFileDataList() const;
    void beginFileUpdate();
    void commitFileUpdate();
    void rollbackFileUpdate();

    // Filter management
    int32_t addFilter(const FilterData& filter);
    void removeFilter(int32_t filterId);
    std::vector<FilterDataPtr> getFilterDataList();
    std::map<int32_t, int32_t> getFilterMatchCounts() const;
    void updateFilterRow(int32_t filterId, int32_t filterRow);
    void updateFilter(const FilterData& filter);
    void beginFilterUpdate();
    void commitFilterUpdate();
    void rollbackFilterUpdate();
    std::string getNextFilterColor();
    void reloadFiles();

    // Search management
    int32_t addSearch(const SearchData& search);
    void removeSearch(int32_t searchId);
    std::vector<SearchDataPtr> getSearchDataList();
    std::map<int32_t, int32_t> getSearchMatchCounts() const;
    void updateSearchRow(int32_t searchId, int32_t searchRow);
    void updateSearch(const SearchData& search);
    void beginSearchUpdate();
    void commitSearchUpdate();
    void rollbackSearchUpdate();
    std::string getNextSearchColor();  

    // Output management
    std::vector<std::shared_ptr<OutputLine>> getOutputStringList() const;
    bool getNextMatchByFilter(int32_t filterId, int32_t lineIndex, int32_t charIndex,
                              int32_t& matchLineIndex, int32_t& matchCharStartIndex, int32_t& matchCharEndIndex);
    bool getPreviousMatchByFilter(int32_t filterId, int32_t lineIndex, int32_t charIndex,
                                  int32_t& matchLineIndex, int32_t& matchCharStartIndex, int32_t& matchCharEndIndex);
    bool getNextMatchBySearch(int32_t searchId, int32_t lineIndex, int32_t charIndex,
                              int32_t& matchLineIndex, int32_t& matchCharStartIndex, int32_t& matchCharEndIndex);
    bool getPreviousMatchBySearch(int32_t searchId, int32_t lineIndex, int32_t charIndex,
                                  int32_t& matchLineIndex, int32_t& matchCharStartIndex, int32_t& matchCharEndIndex);

private:
    int64_t id = -1;
    std::string workspaceName;
    int32_t sortIndex = -1;

    bool m_bActive = false;

    int32_t m_nextFileId = 101;
    std::map<int32_t/*fileId*/, FileDataPtr> m_files;
    bool m_bInFileUpdateTransaction = false;
    int32_t m_nextFilterId = 201;
    std::map<int32_t, FilterDataPtr> m_filters;
    bool m_bInFilterUpdateTransaction = false;

    int32_t m_nextSearchId = 301;
    std::map<int32_t, SearchDataPtr> m_searches;
    bool m_bInSearchUpdateTransaction = false;

    OutputData m_outputData;
    FilterSearchColorManager m_filterSearchColorManager;
};

// Smart pointer type for WorkspaceData
using WorkspaceDataPtr = std::shared_ptr<WorkspaceData>;

} // namespace Core

#endif // CORE_WORKSPACEDATA_H
