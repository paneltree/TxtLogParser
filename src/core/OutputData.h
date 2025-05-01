#pragma once

#include <list>
#include <map>
#include <set>
#include <memory>
#include <vector>
#include <utility>

#include "FileData.h"
#include "FilterData.h"
#include "SearchData.h"
#include "OutputLine.h"
#include "OutputWindow.h"

namespace Core {

    struct FileLineInfo{
        int32_t fileId;
        int32_t fileRow;
        int32_t fileLineIndex;
        std::string lineContent;
        std::string color;

        FileLineInfo(int32_t id, int32_t row, int32_t lineIndex, const std::string& content, const std::string& col)
            : fileId(id)
            , fileRow(row)
            , fileLineIndex(lineIndex)
            , lineContent(content)
            , color(col)
        {}
    };

    /**
     * @brief Pure C++ class representing output data
     * 
     * This class manages the output display data, including file content, filters, and searches.
     */
    class OutputData {
    public:
        OutputData();
        virtual ~OutputData();

    public:
        bool isActive() const;
        void setActive(bool bActive);

        // File management
        void addFile(std::shared_ptr<FileData> file);
        void removeFile(int32_t id);
        void updateFileRow(int32_t id, int32_t row);
        void reloadFiles();

        // Filter management
        void addFilter(std::shared_ptr<FilterData> filter);
        void removeFilter(int32_t id);
        void updateFilterRow(int32_t id, int32_t row);
        void clearFilters();
        void updateFilter(const FilterData& filter);
        std::map<int32_t, int32_t> getFilterMatchCounts() const;
        
        // Search management
        void addSearch(std::shared_ptr<SearchData> search);
        void removeSearch(int32_t id);
        void updateSearchRow(int32_t id, int32_t row);
        void clearSearches();
        void updateSearch(const SearchData& search);
        std::map<int32_t, int32_t> getSearchMatchCounts() const;

        // Display management
        void pauseRefresh();
        void resumeRefresh();
        void refresh();
        std::vector<std::shared_ptr<OutputLine>> getOutputStringList() const;
        
        // Filter navigation
        bool getNextMatchByFilter(int32_t filterId, int32_t lineIndex, int32_t charIndex,
                              int32_t& matchLineIndex, int32_t& matchCharStartIndex, int32_t& matchCharEndIndex);
        bool getPreviousMatchByFilter(int32_t filterId, int32_t lineIndex, int32_t charIndex,
                                  int32_t& matchLineIndex, int32_t& matchCharStartIndex, int32_t& matchCharEndIndex);
        
        // Search navigation
        bool getNextMatchBySearch(int32_t searchId, int32_t lineIndex, int32_t charIndex,
                              int32_t& matchLineIndex, int32_t& matchCharStartIndex, int32_t& matchCharEndIndex);
        bool getPreviousMatchBySearch(int32_t searchId, int32_t lineIndex, int32_t charIndex,
                                  int32_t& matchLineIndex, int32_t& matchCharStartIndex, int32_t& matchCharEndIndex);
    
    protected:
        void loadFile(std::shared_ptr<FileData> file);
        void recreateOutputLines();
        void applyEnabledFilters();
        void applyEnabledSearches();
        void combineFiltersAndSearches();

        void initOutputWindowInfo();
    
    protected:
        bool m_bActive = false;
        std::map<int32_t/*fileId*/, std::shared_ptr<FileData>> m_allFiles;  
        std::map<int32_t/*fileId*/, std::shared_ptr<FileData>> m_loadedFiles;
        std::map<int32_t/*fileId*/, std::vector<std::shared_ptr<FileLineInfo>>> m_allFileLineInfos;

        // Filter management
        std::vector<std::shared_ptr<OutputLine>> m_outputLinesAfterFilters;
        std::map<int32_t/*filterId*/, std::shared_ptr<FilterData>> m_filters;
        std::map<int32_t/*filterRow*/, std::shared_ptr<FilterData>> m_enabledFilters;
        std::map<int32_t/*filterId*/, int32_t/*matchCount*/> m_filterMatchCount;
        std::map<int32_t/*filterId*/, std::set<int32_t/*outputLineIndex*/>> m_filterLineMap;

        // Search management
        std::vector<std::shared_ptr<OutputLine>> m_outputLinesAfterSearches;
        std::map<int32_t/*searchId*/, std::shared_ptr<SearchData>> m_searches;
        std::map<int32_t/*searchRow*/, std::shared_ptr<SearchData>> m_enabledSearches;
        std::map<int32_t/*searchId*/, int32_t/*matchCount*/> m_searchMatchCount;
        std::map<int32_t/*searchId*/, std::set<int32_t/*outputLineIndex*/>> m_searchLineMap;

        // Output data
        std::vector<std::shared_ptr<OutputLine>> m_outputLines;

        OutputWindow m_outputWindow;

        bool m_bRefreshPaused = false;
        bool m_bHasPendingRecreateOutputLines = false;
    };
}
