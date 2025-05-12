#include "OutputData.h"
#include <fstream>
#include "Logger.h"

namespace Core {
    // Define a struct for search matches
    struct SearchMatch {
        int32_t startPos;
        int32_t endPos;
        
        SearchMatch(int32_t start, int32_t end) : startPos(start), endPos(end) {}
    };
    OutputData::OutputData()
        : m_outputWindow(*this)
    {
    }

    OutputData::~OutputData(){

    }

    bool OutputData::isActive() const{
        return m_bActive;
    }

    void OutputData::setActive(bool bActive){
        m_bActive = bActive;
        if(m_bActive){
            pauseRefresh();
            for(auto it : m_allFiles){
                loadFile(it.second);
            }
            resumeRefresh();
            refresh();
        }
    }

    ////////////////////////////////////////////////////////////
    // File management
    ////////////////////////////////////////////////////////////

    void OutputData::addFile(std::shared_ptr<FileData> file){
        m_allFiles[file->getFileId()] = file;
        if(m_bActive){
            loadFile(file);
        }
    }

    void OutputData::removeFile(int32_t id){
        std::map<int32_t, std::shared_ptr<FileData>>::iterator it = m_allFiles.find(id);
        if(it != m_allFiles.end()){
            int32_t fileRow = it->second->getFileRow();
            m_loadedFiles.erase(id);
            auto fileLineInfosIt = m_allFileLineInfos.find(id);
            if(fileLineInfosIt != m_allFileLineInfos.end()){
                m_allFileLineInfos.erase(fileLineInfosIt);
                recreateOutputLines();
            }
            m_allFiles.erase(it);
        }
    }

    void OutputData::updateFileRow(int32_t id, int32_t row){
        recreateOutputLines();
    }

    void OutputData::loadFile(std::shared_ptr<FileData> file){
        if(m_loadedFiles.find(file->getFileId()) != m_loadedFiles.end()){
            return;
        }
        m_loadedFiles[file->getFileId()] = file;
        std::vector<std::shared_ptr<FileLineInfo>>& fileLineInfos = m_allFileLineInfos[file->getFileId()];
        fileLineInfos.clear();
        std::ifstream fileStream(file->getPath());
        std::string line;
        int32_t lineIndex = 0;
        while (std::getline(fileStream, line)) {
            //remove the last \n or \r\n
            if(!line.empty() && line.back() == '\n'){
                line.pop_back();
            }
            if(!line.empty() && line.back() == '\r'){
                line.pop_back();
            }
            //replay \r in the line with ' '
            for(int32_t i = 0; i < line.size(); i++){
                if(line[i] == '\r'){
                    line[i] = ' ';
                }
            }
            fileLineInfos.push_back(std::make_shared<FileLineInfo>(file->getFileId(), file->getFileRow(), lineIndex++, line, ""));
        }
        if(!fileLineInfos.empty()){
            recreateOutputLines();
        }
    }

    void OutputData::reloadFiles(){
        pauseRefresh();
        m_loadedFiles.clear();
        m_allFileLineInfos.clear();
        for(auto it : m_allFiles){
            if(it.second->isSelected()){
                loadFile(it.second);
            }
        }
        recreateOutputLines();
        resumeRefresh();
        refresh();
    }
    ////////////////////////////////////////////////////////////
    // Filter management
    ////////////////////////////////////////////////////////////

    void OutputData::addFilter(std::shared_ptr<FilterData> filter){
        m_filters[filter->getId()] = filter;
        if(filter->isEnabled()){
            m_enabledFilters[filter->getRow()] = filter;
            recreateOutputLines();
        }
    }

    void OutputData::removeFilter(int32_t id){
        auto it = m_filters.find(id);
        if(it != m_filters.end()){
            auto filterRow = it->second->getRow();
            if(m_enabledFilters.find(filterRow) != m_enabledFilters.end()){
                m_enabledFilters.erase(filterRow);
                recreateOutputLines();
            }
            m_filters.erase(it);
        }
    }

    void OutputData::refreshByFilterRowsChanged(){
        m_enabledFilters.clear();
        for(auto it : m_filters){
            if(it.second->isEnabled()){
                m_enabledFilters[it.second->getRow()] = it.second;
            }
        }
        recreateOutputLines();
    }

    void OutputData::clearFilters(){
        m_filters.clear();
        if(m_enabledFilters.size() > 0){
            m_enabledFilters.clear();
            recreateOutputLines();
        }
    }

    void OutputData::updateFilter(const FilterData& filter){
        auto it = m_filters.find(filter.getId());
        if(it != m_filters.end()){  
            it->second->update(filter);
            if(filter.isEnabled()){
                m_enabledFilters[filter.getRow()] = it->second;
            }else{
                m_enabledFilters.erase(filter.getRow());
            }
            recreateOutputLines();
        }
    }

    std::map<int32_t, int32_t> OutputData::getFilterMatchCounts() const{
        return m_filterMatchCount;
    }

    ////////////////////////////////////////////////////////////
    // Search management
    ////////////////////////////////////////////////////////////

    void OutputData::addSearch(std::shared_ptr<SearchData> search){
        m_searches[search->getId()] = search;
        if(search->isEnabled()){
            m_enabledSearches[search->getRow()] = search;
            recreateOutputLines();
        }
    }

    void OutputData::removeSearch(int32_t id){
        auto it = m_searches.find(id);  
        if(it != m_searches.end()){
            auto searchRow = it->second->getRow();
            if(m_enabledSearches.find(searchRow) != m_enabledSearches.end()){
                m_enabledSearches.erase(searchRow);
                recreateOutputLines();
            }
            m_searches.erase(it);
        }
    }

    void OutputData::refreshBySearchRowsChanged(){
        m_enabledSearches.clear();
        for(auto it : m_searches){
            if(it.second->isEnabled()){
                m_enabledSearches[it.second->getRow()] = it.second;
            }
        }
        recreateOutputLines();
    }

    void OutputData::clearSearches(){
        m_searches.clear();
        if(m_enabledSearches.size() > 0){
            m_enabledSearches.clear();
            recreateOutputLines();
        }
    }

    void OutputData::updateSearch(const SearchData& search){
        auto it = m_searches.find(search.getId());
        if(it != m_searches.end()){
            it->second->update(search);
            if(search.isEnabled()){
                m_enabledSearches[search.getRow()] = it->second;
            }else{
                m_enabledSearches.erase(search.getRow());
            }
            recreateOutputLines();
        }
    }

    std::map<int32_t, int32_t> OutputData::getSearchMatchCounts() const{
        return m_searchMatchCount;
    }

    ////////////////////////////////////////////////////////////
    // Display management
    ////////////////////////////////////////////////////////////

    void OutputData::pauseRefresh(){
        m_bRefreshPaused = true;
    }

    void OutputData::resumeRefresh(){
        m_bRefreshPaused = false;
    }

    void OutputData::refresh(){
        if(m_bRefreshPaused){
            return;
        }
        if(m_bHasPendingRecreateOutputLines){
            recreateOutputLines();
        }
    }

    void OutputData::recreateOutputLines(){
        if(m_bRefreshPaused){
            m_bHasPendingRecreateOutputLines = true;
            return;
        }
        m_bHasPendingRecreateOutputLines = false;
        m_outputLines.clear();
        m_outputLinesAfterFilters.clear();
        m_outputLinesAfterSearches.clear();
        m_filterMatchCount.clear();
        m_filterLineMap.clear();
        m_searchMatchCount.clear();
        m_searchLineMap.clear();
        // Apply filters first
        applyEnabledFilters();
        // Then apply searches
        applyEnabledSearches();
        combineFiltersAndSearches();
        m_outputWindow.setLinesCount(m_outputLines.size());
        (Logger::getInstance() << "Recreating output lines, total lines: " << m_outputLines.size()).info();
    }

    void OutputData::applyEnabledFilters(){
        //loop m_allFileLineInfos, apply filters
        std::map<int32_t/*fileRow*/, int32_t/*fileId*/> fileRowToId;
        //sort the fileIds by fileRow
        for(auto it: m_allFileLineInfos){
            auto fileId = it.first;
            auto fileRow = m_allFiles[fileId]->getFileRow();
            fileRowToId[fileRow] = fileId;
        }
        for(auto it : fileRowToId){
            auto fileId = it.second;
            auto fileRow = it.first;
            for(auto itLine : m_allFileLineInfos[fileId]){
                std::shared_ptr<OutputLine> outputLine = std::make_shared<OutputLine>();
                outputLine->setFileId(fileId);
                outputLine->setFileRow(fileRow);
                outputLine->setLineIndex(itLine->fileLineIndex);
                outputLine->setContent(std::string_view(itLine->lineContent));

                std::list<OutputSubLine> subLines;
                OutputSubLine subLine;
                subLine.setContent(std::string_view(itLine->lineContent));
                subLines.push_back(subLine);

                if(!m_enabledFilters.empty()){
                    for(auto itFilter : m_enabledFilters){
                        std::list<OutputSubLine> subLines2;
                        for(auto& subLine : subLines){
                            if(subLine.getFilterId() != -1){
                                subLines2.push_back(subLine);
                            }else{
                                itFilter.second->apply(subLine.getContent(), subLines2);
                            }
                        }
                        subLines = subLines2;
                    }
                    bool matched = false;
                    int32_t outputLineIndex = (int32_t)m_outputLinesAfterFilters.size();
                    int32_t outputSubLineIndex = 0;
                    for(auto& subLine : subLines){
                        if(subLine.getFilterId() != -1){
                            matched = true;
                            m_filterMatchCount[subLine.getFilterId()]++;
                            m_filterLineMap[subLine.getFilterId()].insert(outputLineIndex);
                        }
                        outputLine->addSubLine(subLine);
                        outputSubLineIndex++;
                    }
                    if(matched){
                        m_outputLinesAfterFilters.push_back(outputLine);
                    }
                }else
                {
                    for(auto& subLine : subLines){
                        outputLine->addSubLine(subLine);
                    }
                    m_outputLinesAfterFilters.push_back(outputLine);
                }
            }
        }
    }
    
    
    void OutputData::applyEnabledSearches() {
        for(auto& filteredLine : m_outputLinesAfterFilters){
            auto fileId = filteredLine->getFileId();
            auto fileRow = filteredLine->getFileRow();
            auto lineIndex = filteredLine->getLineIndex();

            auto& fileLines = m_allFileLineInfos[fileId];
            assert(lineIndex < fileLines.size());
            auto& fileLine = fileLines[lineIndex];

            std::shared_ptr<OutputLine> outputLine = std::make_shared<OutputLine>();
            outputLine->setFileId(fileId);
            outputLine->setFileRow(fileRow);
            outputLine->setLineIndex(lineIndex);

            std::list<OutputSubLine> subLines;
            OutputSubLine subLine;
            subLine.setContent(std::string_view(fileLine->lineContent));
            subLines.push_back(subLine);

            if(!m_enabledSearches.empty()){
                for(auto itSearch : m_enabledSearches){
                    std::list<OutputSubLine> subLines2;
                    for(auto& subLine : subLines){
                        if(subLine.getSearchId() != -1){
                            subLines2.push_back(subLine);
                        }else{
                            itSearch.second->apply(subLine.getContent(), subLines2);
                        }
                    }
                    subLines = subLines2;
                }
                bool matched = false;
                int32_t outputLineIndex = (int32_t)m_outputLinesAfterSearches.size();
                int32_t outputSubLineIndex = 0;
                for(auto& subLine : subLines){
                    if(subLine.getSearchId() != -1){
                        matched = true;
                        m_searchMatchCount[subLine.getSearchId()]++;
                        m_searchLineMap[subLine.getSearchId()].insert(outputLineIndex);
                    }
                    outputLine->addSubLine(subLine);
                    outputSubLineIndex++;
                }
                m_outputLinesAfterSearches.push_back(outputLine);
            }else{
                for(auto& subLine : subLines){
                    outputLine->addSubLine(subLine);
                }
                m_outputLinesAfterSearches.push_back(outputLine);
            }
        }
    }

    void OutputData::combineFiltersAndSearches(){
        m_outputLines.clear();
        assert(m_outputLinesAfterFilters.size() == m_outputLinesAfterSearches.size());
        for(int32_t i = 0; i < m_outputLinesAfterFilters.size(); i++){
            auto& filteredLine = m_outputLinesAfterFilters[i];
            auto& searchedLine = m_outputLinesAfterSearches[i];

            auto& filteredSubLines = filteredLine->getSubLines();
            auto& searchedSubLines = searchedLine->getSubLines();
            
            // Create a new OutputLine for the result
            auto combinedLine = std::make_shared<OutputLine>();
            combinedLine->setFileId(filteredLine->getFileId());
            combinedLine->setFileRow(filteredLine->getFileRow());
            combinedLine->setLineIndex(filteredLine->getLineIndex());
            
            // If there's no sublines in either, just continue
            if (filteredSubLines.empty() && searchedSubLines.empty()) {
                m_outputLines.push_back(combinedLine);
                continue;
            }
            
            // If there's no search sublines, just use filter sublines
            if (searchedSubLines.empty()) {
                for (auto& subLine : filteredSubLines) {
                    combinedLine->addSubLine(subLine);
                }
                m_outputLines.push_back(combinedLine);
                continue;
            }
            
            // If there's no filter sublines, just use search sublines
            if (filteredSubLines.empty()) {
                for (auto& subLine : searchedSubLines) {
                    combinedLine->addSubLine(subLine);
                }
                m_outputLines.push_back(combinedLine);
                continue;
            }
            
            std::list<OutputSubLine> combinedSubLines;
            for(auto& filteredSubLine : filteredSubLines){
                combinedSubLines.push_back(filteredSubLine);
            }
            for(auto& searchedSubLine : searchedSubLines){
                if(searchedSubLine.getSearchId() == -1){
                    continue;
                }
                std::string_view searchContent = searchedSubLine.getContent();
                const char* search_first = searchContent.data();
                const char* search_last = search_first + searchContent.size() - 1;
                
                std::list<OutputSubLine> combinedSubLines2;
                for(auto& combinedSubLine : combinedSubLines){
                    std::string_view combinedContent = combinedSubLine.getContent();
                    const char* combined_first = combinedContent.data();
                    const char* combined_last = combined_first + combinedContent.size() - 1;
                    if(search_first > combined_last){
                        combinedSubLines2.push_back(combinedSubLine);
                        continue;
                    }
                    if(search_last < combined_first){
                        combinedSubLines2.push_back(combinedSubLine);
                        continue;
                    }
                    size_t total_size = combinedContent.size();
                    size_t left_combined_size = 0;
                    size_t middle_searched_size = 0;
                    size_t right_combined_size = 0;

                    const char* middle_searched_first = nullptr;
                    const char* middle_searched_last = nullptr;
                    if(combined_first < search_first){
                        left_combined_size = search_first - combined_first;
                        middle_searched_first = search_first;
                    }else{
                        middle_searched_first = combined_first;
                    }
                    if(combined_last > search_last){
                        right_combined_size = combined_last - search_last;
                        middle_searched_last = search_last;
                    }else{
                        middle_searched_last = combined_last;
                    }
                    assert(nullptr != middle_searched_first);
                    assert(nullptr != middle_searched_last);
                    middle_searched_size = total_size - left_combined_size - right_combined_size;
                    size_t middle_searched_size2 = middle_searched_last - middle_searched_first + 1;
                    assert(middle_searched_size == middle_searched_size2);
                    if(0 < left_combined_size){
                        OutputSubLine leftSubLine = combinedSubLine;
                        leftSubLine.setContent(std::string_view(combined_first, left_combined_size));
                        combinedSubLines2.push_back(leftSubLine);
                    }
                    if(0 < middle_searched_size){
                        OutputSubLine middleSubLine = searchedSubLine;
                        middleSubLine.setContent(std::string_view(middle_searched_first, middle_searched_size));
                        combinedSubLines2.push_back(middleSubLine);
                    }
                    if(0 < right_combined_size){
                        OutputSubLine rightSubLine = combinedSubLine;
                        rightSubLine.setContent(std::string_view(combined_first + left_combined_size + middle_searched_size, right_combined_size));
                        combinedSubLines2.push_back(rightSubLine);
                    }
                }
                combinedSubLines = combinedSubLines2;
            }
            for(auto& subLine : combinedSubLines){
                combinedLine->addSubLine(subLine);
            }
            m_outputLines.push_back(combinedLine);
        }
    }
    
       
    std::vector<std::shared_ptr<OutputLine>> OutputData::getOutputStringList() const {
        std::vector<std::shared_ptr<OutputLine>> result;
        if(m_outputWindow.getTotalLines() == 0){
            return result;
        }
        int32_t topIndex = m_outputWindow.getVisiableTopLineIndex();
        int32_t bottomIndex = m_outputWindow.getVisiableBottomLineIndex();
        if(0 > topIndex || 0 > bottomIndex){
            return result;
        }
        if(topIndex > bottomIndex){
            return result;
        }
        if(topIndex >= m_outputLines.size() || bottomIndex >= m_outputLines.size()){
            return result;
        }
        for(int32_t i = topIndex; i <= bottomIndex; i++){
            result.push_back(m_outputLines[i]);
        }
        return result;
    }

    bool OutputData::getNextMatchByFilter(int32_t filterId, int32_t lineIndex, int32_t charIndex,
                              int32_t& matchLineIndex, int32_t& matchCharStartIndex, int32_t& matchCharEndIndex){
        auto it = m_filterLineMap.find(filterId);
        if(it == m_filterLineMap.end()){
            return false;
        }
        auto& lineSet = it->second;
        if(lineSet.empty()){
            return false;
        }
        auto it2 = lineSet.find(lineIndex);
        if(it2 != lineSet.end()){
            int32_t outputLineIndex = *it2;
            auto& outputLine = m_outputLinesAfterFilters[outputLineIndex];
            int32_t outputLineCharIndex = 0;
            for(auto& subLine : outputLine->getSubLines()){
                if(outputLineCharIndex < charIndex){
                    outputLineCharIndex += subLine.getContent().size();
                    continue;
                }
                if(subLine.getFilterId() == filterId){
                    matchLineIndex = outputLineIndex;
                    matchCharStartIndex = outputLineCharIndex;
                    matchCharEndIndex = outputLineCharIndex + subLine.getContent().size();
                    return true;
                }
                outputLineCharIndex += subLine.getContent().size();
            }
        }
        it2 = lineSet.upper_bound(lineIndex);
        if(it2 == lineSet.end()){
            it2 = lineSet.begin();
        }
        if(it2 != lineSet.end()){
            int32_t outputLineIndex = *it2;
            auto& outputLine = m_outputLinesAfterFilters[outputLineIndex];
            int32_t outputLineCharIndex = 0;
            for(auto& subLine : outputLine->getSubLines()){
                if(subLine.getFilterId() == filterId){
                    matchLineIndex = outputLineIndex;
                    matchCharStartIndex = outputLineCharIndex;
                    matchCharEndIndex = outputLineCharIndex + subLine.getContent().size();
                    return true;
                }
                outputLineCharIndex += subLine.getContent().size();
            }
        }

        return false;
    }

    bool OutputData::getPreviousMatchByFilter(int32_t filterId, int32_t lineIndex, int32_t charIndex,
                              int32_t& matchLineIndex, int32_t& matchCharStartIndex, int32_t& matchCharEndIndex){
        auto it = m_filterLineMap.find(filterId);
        if(it == m_filterLineMap.end()){
            return false;
        }
        auto& lineSet = it->second;
        if(lineSet.empty()){
            return false;
        }
        auto it2 = lineSet.find(lineIndex);
        if(it2 != lineSet.end()){
            int32_t outputLineIndex = *it2;
            auto& outputLine = m_outputLinesAfterFilters[outputLineIndex];
            int32_t outputLineCharIndex = 0;
            auto& subLines = outputLine->getSubLines();
            for(auto& subLine : subLines){
                outputLineCharIndex += subLine.getContent().size();
            }
            for(auto it = subLines.rbegin(); it != subLines.rend(); it++){
                outputLineCharIndex -= it->getContent().size();
                if(outputLineCharIndex >= charIndex){
                    continue;
                }
                if(it->getFilterId() == filterId){
                    matchLineIndex = outputLineIndex;
                    matchCharStartIndex = outputLineCharIndex;
                    matchCharEndIndex = outputLineCharIndex + it->getContent().size();
                    return true;
                }
            }
        }
        it2 = lineSet.lower_bound(lineIndex);
        if(it2 == lineSet.end()){
            //it2 is the lastest line
            it2 = std::prev(lineSet.end());
        }else{
            if(it2 != lineSet.begin()){
                it2--;
            }else{
                //it2 is the latest line
                it2 = std::prev(lineSet.end());
            }
        }
        if(it2 != lineSet.end()){
            int32_t outputLineIndex = *it2;
            auto& outputLine = m_outputLinesAfterFilters[outputLineIndex];
            //loop in reverse order
            auto& subLines = outputLine->getSubLines();
            int32_t outputLineCharIndex = 0;
            for(auto& subLine : subLines){
                outputLineCharIndex += subLine.getContent().size();
            }
            for(auto it = subLines.rbegin(); it != subLines.rend(); it++){
                outputLineCharIndex -= it->getContent().size();
                if(it->getFilterId() == filterId){
                    matchLineIndex = outputLineIndex;
                    matchCharStartIndex = outputLineCharIndex;
                    matchCharEndIndex = outputLineCharIndex + it->getContent().size();
                    return true;    
                }
            }
        }
        return false;
    }

    bool OutputData::getNextMatchBySearch(int32_t searchId, int32_t lineIndex, int32_t charIndex,
                                       int32_t& matchLineIndex, int32_t& matchCharStartIndex, int32_t& matchCharEndIndex) {
        auto it = m_searchLineMap.find(searchId);
        if(it == m_searchLineMap.end()){
            return false;
        }
        auto& lineSet = it->second;
        if(lineSet.empty()){
            return false;
        }
        auto it2 = lineSet.find(lineIndex);
        if(it2 != lineSet.end()){
            int32_t outputLineIndex = *it2;
            auto& outputLine = m_outputLinesAfterSearches[outputLineIndex];
            int32_t outputLineCharIndex = 0;
            for(auto& subLine : outputLine->getSubLines()){
                if(outputLineCharIndex < charIndex){
                    outputLineCharIndex += subLine.getContent().size();
                    continue;
                }
                if(subLine.getSearchId() == searchId){
                    matchLineIndex = outputLineIndex;
                    matchCharStartIndex = outputLineCharIndex;
                    matchCharEndIndex = outputLineCharIndex + subLine.getContent().size();
                    return true;
                }
                outputLineCharIndex += subLine.getContent().size();
            }
        }
        it2 = lineSet.upper_bound(lineIndex);
        if(it2 == lineSet.end()){
            it2 = lineSet.begin();
        }
        if(it2 != lineSet.end()){
            int32_t outputLineIndex = *it2;
            auto& outputLine = m_outputLinesAfterSearches[outputLineIndex];
            int32_t outputLineCharIndex = 0;
            for(auto& subLine : outputLine->getSubLines()){
                if(subLine.getSearchId() == searchId){
                    matchLineIndex = outputLineIndex;
                    matchCharStartIndex = outputLineCharIndex;
                    matchCharEndIndex = outputLineCharIndex + subLine.getContent().size();
                    return true;
                }
                outputLineCharIndex += subLine.getContent().size();
            }
        }

        return false;
    }
    
    bool OutputData::getPreviousMatchBySearch(int32_t searchId, int32_t lineIndex, int32_t charIndex,
                                          int32_t& matchLineIndex, int32_t& matchCharStartIndex, int32_t& matchCharEndIndex) {
        auto it = m_searchLineMap.find(searchId);
        if(it == m_searchLineMap.end()){
            return false;
        }
        auto& lineSet = it->second;
        if(lineSet.empty()){
            return false;
        }
        auto it2 = lineSet.find(lineIndex);
        if(it2 != lineSet.end()){
            int32_t outputLineIndex = *it2;
            auto& outputLine = m_outputLinesAfterSearches[outputLineIndex];
            int32_t outputLineCharIndex = 0;
            auto& subLines = outputLine->getSubLines();
            for(auto& subLine : subLines){
                outputLineCharIndex += subLine.getContent().size();
            }
            for(auto it = subLines.rbegin(); it != subLines.rend(); it++){
                outputLineCharIndex -= it->getContent().size();
                if(outputLineCharIndex >= charIndex){
                    continue;
                }
                if(it->getSearchId() == searchId){
                    matchLineIndex = outputLineIndex;
                    matchCharStartIndex = outputLineCharIndex;
                    matchCharEndIndex = outputLineCharIndex + it->getContent().size();
                    return true;
                }
            }
        }
        it2 = lineSet.lower_bound(lineIndex);
        if(it2 == lineSet.end()){
            //it2 is the lastest line
            it2 = std::prev(lineSet.end());
        }else{
            if(it2 != lineSet.begin()){
                it2--;
            }else{
                //it2 is the latest line
                it2 = std::prev(lineSet.end());
            }
        }
        if(it2 != lineSet.end()){
            int32_t outputLineIndex = *it2;
            auto& outputLine = m_outputLinesAfterSearches[outputLineIndex];
            //loop in reverse order
            auto& subLines = outputLine->getSubLines();
            int32_t outputLineCharIndex = 0;
            for(auto& subLine : subLines){
                outputLineCharIndex += subLine.getContent().size();
            }
            for(auto it = subLines.rbegin(); it != subLines.rend(); it++){
                outputLineCharIndex -= it->getContent().size();
                if(it->getSearchId() == searchId){
                    matchLineIndex = outputLineIndex;
                    matchCharStartIndex = outputLineCharIndex;
                    matchCharEndIndex = outputLineCharIndex + it->getContent().size();
                    return true;    
                }
            }
        }
        return false;
    }

}
