#ifndef SEARCHDATA_H
#define SEARCHDATA_H

#include <string>
#include <memory>
#include <list>
#include <vector>
#include "Logger.h"
#include "OutputLine.h"
#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace Core {


/**
 * @brief Pure C++ class representing search data
 * 
 * This class replaces the Qt-dependent SearchConfig class with a pure C++ implementation.
 */
class SearchData {
public:
    SearchData();
        
    SearchData( int32_t id,
                int32_t row,
                const std::string& pattern,
                bool caseSensitive,
                bool wholeWord,
                bool regex,
                bool enabled,
                const std::string& color = "");

    bool saveToJson(json& j) const;
    bool loadFromJson(const json& j);

    int32_t getId() const { return m_searchId; }
    void setId(int32_t id) { m_searchId = id; }

    int32_t getRow() const { return m_searchRow; }
    void setRow(int32_t row) { m_searchRow = row; }
    
    // Getters and setters
    const std::string& getPattern() const { return m_searchPattern; }
    void setPattern(const std::string& pattern) { m_searchPattern = pattern; }
    
    bool isCaseSensitive() const { return m_caseSensitive; }
    void setCaseSensitive(bool value) { m_caseSensitive = value; }
    
    bool isWholeWord() const { return m_wholeWord; }
    void setWholeWord(bool value) { m_wholeWord = value; }
    
    bool isRegex() const { return m_regex; }
    void setRegex(bool value) { m_regex = value;}
    
    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool value) { m_enabled = value;}
    
    const std::string& getColor() const { return m_colorString; }
    void setColor(const std::string& color) { m_colorString = color;  }

    void update(const SearchData& search, bool* pChanged = nullptr);

    void apply(const std::string_view& lineContent, std::list<OutputSubLine>& sublines);
    void applyNonRegex(const std::string_view& lineContent, std::list<OutputSubLine>& sublines);
    void applyRegex(const std::string_view& lineContent, std::list<OutputSubLine>& sublines);
private:
    int32_t m_searchId;
    int32_t m_searchRow;
    std::string m_searchPattern;
    bool m_caseSensitive = false;
    bool m_wholeWord = false;
    bool m_regex = false;
    bool m_enabled = true;
    std::string m_colorString;
};

// Smart pointer type for SearchData
using SearchDataPtr = std::shared_ptr<SearchData>;

} // namespace Core

#endif // SEARCHDATA_H 