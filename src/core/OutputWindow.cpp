#include "OutputWindow.h"

#include "OutputData.h"

namespace Core{
    OutputWindow::OutputWindow(OutputData& outputData)
        : m_outputData(outputData)
    {
        reset();
    }

    OutputWindow::~OutputWindow(){
    }

    void OutputWindow::reset(){
        m_totalLines = 0;
        m_currentLineIndex = -1;
        m_visiableTopLineIndex = -1;
        m_visiableBottomLineIndex = -1;
    }

    void OutputWindow::setLinesCount(int32_t lineCount){
        m_totalLines = lineCount;
        if(0 >= m_totalLines){
            m_totalLines = 0;
            m_currentLineIndex = -1;
            m_visiableTopLineIndex = -1;
            m_visiableBottomLineIndex = -1;
            return;
        }
        if(m_currentLineIndex < 0){
            m_currentLineIndex = 0;
        }
        if(m_visiableTopLineIndex < 0){
            m_visiableTopLineIndex = 0;
        }
        m_visiableBottomLineIndex = m_visiableTopLineIndex + m_visiableLineCount - 1;
        if(m_visiableBottomLineIndex >= m_totalLines){
            m_visiableBottomLineIndex = m_totalLines - 1;
            m_visiableTopLineIndex = m_visiableBottomLineIndex - m_visiableLineCount + 1;
            if(m_visiableTopLineIndex < 0){
                m_visiableTopLineIndex = 0;
            }
        }  
    }

    void OutputWindow::clearAllLines(){
        reset();
    }
}