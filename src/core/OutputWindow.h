#pragma once

#include <cstdint>

namespace Core{
    class OutputData;

    class OutputWindow{
    public:
        OutputWindow(OutputData& outputData);
        virtual ~OutputWindow();

    public:
        void setLinesCount(int32_t lineCount);
        void clearAllLines();

        int32_t getVisiableLineCount() const { return m_visiableLineCount; }
        int32_t getTotalLines() const { return m_totalLines; }
        int32_t getVisiableTopLineIndex() const { return m_visiableTopLineIndex; }
        int32_t getVisiableBottomLineIndex() const { return m_visiableBottomLineIndex; }
        int32_t getCurrentLineIndex() const { return m_currentLineIndex; }        
        
    protected:
        void reset();
    private:
        OutputData& m_outputData;
        int32_t m_visiableLineCount = 100000;       //number of lines visible in the output window
        int32_t m_totalLines;                   //total lines in the output window
        int32_t m_visiableTopLineIndex;         //top visible line index in the output window, between 0 and total lines - 1
        int32_t m_visiableBottomLineIndex;      //bottom visible line index in the output window, between 0 and total lines - 1
        int32_t m_currentLineIndex;             //current line index in the output window, between top and bottom visible line index
    };
}
