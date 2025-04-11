#ifndef FILTERDATA_H
#define FILTERDATA_H

#include <string>
#include <memory>
#include <list>
#include <vector>
#include "Logger.h"
#include "OutputLine.h"
#include <nlohmann/json.hpp>
#include <string_view>
using json = nlohmann::json;

namespace Core {


/**
 * @brief Pure C++ class representing filter data
 * 
 * This class replaces the Qt-dependent FilterConfig class with a pure C++ implementation.
 */
class FilterData {
public:
    FilterData();
        
    FilterData( int32_t id,
                int32_t row,
                const std::string& pattern,
                bool caseSensitive,
                bool wholeWord,
                bool regex,
                bool enabled,
                const std::string& color = "");

    bool saveToJson(json& j) const;
    bool loadFromJson(const json& j);

    int32_t getId() const { return m_filterId; }
    void setId(int32_t id) { m_filterId = id; }

    int32_t getRow() const { return m_filterRow; }
    void setRow(int32_t row) { m_filterRow = row; }
    
    // Getters and setters
    const std::string& getPattern() const { return m_filterPattern; }
    void setPattern(const std::string& pattern) { m_filterPattern = pattern; }
    
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

    void update(const FilterData& filter, bool* pChanged = nullptr);

    void apply( const std::string_view& lineContent, std::list<OutputSubLine>& sublines);
    void applyNonRegex( const std::string_view& lineContent, std::list<OutputSubLine>& sublines);
    void applyRegex( const std::string_view& lineContent, std::list<OutputSubLine>& sublines);
private:
    int32_t m_filterId;
    int32_t m_filterRow;
    std::string m_filterPattern;
    bool m_caseSensitive = false;
    bool m_wholeWord = false;
    bool m_regex = false;
    bool m_enabled = true;
    std::string m_colorString;
};

// Smart pointer type for FilterData
using FilterDataPtr = std::shared_ptr<FilterData>;

} // namespace Core

#endif // FILTERDATA_H 