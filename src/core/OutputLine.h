#pragma once

#include <string>
#include <list>
#include <string_view>

namespace Core {

    class OutputSubLine{
    public:
        OutputSubLine();
        void setFileId(int32_t fileId);
        int32_t getFileId() const;
        void setContent(std::string_view content);
        std::string_view getContent() const;

        void setColor(const std::string& color);
        const std::string& getColor() const;

        void setFilterId(int32_t filterId);
        int32_t getFilterId() const;
        void setFilterRow(int32_t filterRow);
        int32_t getFilterRow() const;

        void setSearchId(int32_t searchId);
        int32_t getSearchId() const;
        void setSearchRow(int32_t searchRow);
        int32_t getSearchRow() const;
    private:
        int32_t m_fileId;
        std::string_view m_content;
        std::string m_color;

        int32_t m_filterId = -1;
        int32_t m_filterRow = -1;
        int32_t m_searchId = -1;
        int32_t m_searchRow = -1;
    };

    class OutputLine {
    public:
        OutputLine();
        void setFileId(int32_t fileId);
        void setFileRow(int32_t fileRow);
        void setLineIndex(int32_t lineIndex);
        void setContent(std::string_view content);
        int32_t getFileId() const;
        int32_t getFileRow() const;
        int32_t getLineIndex() const;
        std::string_view getContent() const;
        void addSubLine(const OutputSubLine& subLine);
        const std::list<OutputSubLine>& getSubLines() const;
    private:
        int32_t m_fileId;
        int32_t m_fileRow;
        int32_t m_lineIndex;
        std::string_view m_content;
        std::list<OutputSubLine> m_subLines;
    };
}