#include "OutputLine.h"

namespace Core {
    OutputSubLine::OutputSubLine(){}
    void OutputSubLine::setFileId(int32_t fileId){
        m_fileId = fileId;
    }
    int32_t OutputSubLine::getFileId() const{
        return m_fileId;
    }
    void OutputSubLine::setContent(std::string_view content){
        m_content = content;
    }
    std::string_view OutputSubLine::getContent() const{
        return m_content;
    }
    void OutputSubLine::setColor(const std::string& color){
        m_color = color;
    }
    const std::string& OutputSubLine::getColor() const{
        return m_color;
    }
    void OutputSubLine::setFilterId(int32_t filterId){
        m_filterId = filterId;
    }
    int32_t OutputSubLine::getFilterId() const{
        return m_filterId;
    }
    void OutputSubLine::setFilterRow(int32_t filterRow){
        m_filterRow = filterRow;
    }
    int32_t OutputSubLine::getFilterRow() const{
        return m_filterRow;
    }
    void OutputSubLine::setSearchId(int32_t searchId){
        m_searchId = searchId;
    }
    int32_t OutputSubLine::getSearchId() const{
        return m_searchId;
    }
    void OutputSubLine::setSearchRow(int32_t searchRow){
        m_searchRow = searchRow;
    }
    int32_t OutputSubLine::getSearchRow() const{
        return m_searchRow;
    }

    OutputLine::OutputLine(){}
    void OutputLine::setFileId(int32_t fileId){
        m_fileId = fileId;
    }
    void OutputLine::setFileRow(int32_t fileRow){
        m_fileRow = fileRow;
    }
    void OutputLine::setLineIndex(int32_t lineIndex){
        m_lineIndex = lineIndex;
    }
    void OutputLine::setContent(std::string_view content){
        m_content = content;
    }
    int32_t OutputLine::getFileId() const{
        return m_fileId;
    }
    int32_t OutputLine::getFileRow() const{
        return m_fileRow;
    }
    int32_t OutputLine::getLineIndex() const{
        return m_lineIndex;
    }
    std::string_view OutputLine::getContent() const{
        return m_content;
    }
    void OutputLine::addSubLine(const OutputSubLine& subLine){
        m_subLines.push_back(subLine);
    }
    const std::list<OutputSubLine>& OutputLine::getSubLines() const{
        return m_subLines;
    }
}