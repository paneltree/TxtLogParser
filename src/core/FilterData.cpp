#include "FilterData.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <chrono>
#include <regex>
#include <iomanip>
#include <nlohmann/json.hpp>
#include "AppUtils.h"
#include "Logger.h"

namespace Core {

    FilterData::FilterData(){
        m_filterId = -1;
        m_filterRow = -1;
        m_colorString = "";
        Logger::getInstance().debug("Created new filter with color: " + m_colorString);
    }

    FilterData::FilterData( int32_t id,
                            int32_t row,
                            const std::string& pattern,
                            bool caseSensitive,
                            bool wholeWord,
                            bool regex,
                            bool enabled,
                            const std::string& color) 
        :   m_filterId(id),
            m_filterRow(row),
            m_filterPattern(pattern),
            m_caseSensitive(caseSensitive),
            m_wholeWord(wholeWord),
            m_regex(regex),
            m_enabled(enabled) {
            m_colorString = color;
    }

    bool FilterData::saveToJson(json& j) const
    {
        j["id"] = m_filterId;
        j["row"] = m_filterRow;
        j["pattern"] = m_filterPattern;
        j["caseSensitive"] = m_caseSensitive;
        j["wholeWord"] = m_wholeWord;
        j["regex"] = m_regex;
        j["enabled"] = m_enabled;
        j["color"] = m_colorString;
        return true;

    }
    bool FilterData::loadFromJson(const json& j)
    {
        m_filterId = j.value("id", -1);
        m_filterRow = j.value("row", -1);
        m_filterPattern = j.value("pattern", "");
        m_caseSensitive = j.value("caseSensitive", false);
        m_wholeWord = j.value("wholeWord", false);
        m_regex = j.value("regex", false);
        m_enabled = j.value("enabled", true);
        m_colorString = j.value("color", "");
        return true;        
    }

    void FilterData::update(const FilterData& filter, bool* pChanged){
        bool changed = false;
        assert(m_filterId == filter.m_filterId);
        assert(m_filterRow == filter.m_filterRow);  
        if(m_filterPattern != filter.m_filterPattern){
            m_filterPattern = filter.m_filterPattern;
            changed = true;
        }
        if(m_caseSensitive != filter.m_caseSensitive){
            m_caseSensitive = filter.m_caseSensitive;
            changed = true;
        }
        if(m_wholeWord != filter.m_wholeWord){
            m_wholeWord = filter.m_wholeWord;
            changed = true;
        }
        if(m_regex != filter.m_regex){
            m_regex = filter.m_regex;
            changed = true;
        }
        if(m_enabled != filter.m_enabled){
            m_enabled = filter.m_enabled;   
            changed = true;
        }
        if(m_colorString != filter.m_colorString){
            m_colorString = filter.m_colorString;
            changed = true;
        }   
        if(pChanged != nullptr){
            *pChanged = changed;
        }
    }

    void FilterData::apply(const std::string_view& lineContent, std::list<OutputSubLine>& sublines){
        if(m_enabled){
            if(!m_regex){
                applyNonRegex(lineContent, sublines);
            }
            else{
                applyRegex(lineContent, sublines);
            }
        }
    }

    void FilterData::applyNonRegex(const std::string_view& lineContent, std::list<OutputSubLine>& sublines){
        // Convert pattern and content to lowercase if case insensitive
        std::string pattern = m_filterPattern;
        std::string content = std::string(lineContent);
        if (!m_caseSensitive) {
            std::transform(pattern.begin(), pattern.end(), pattern.begin(), ::tolower);
            std::transform(content.begin(), content.end(), content.begin(), ::tolower);
        }

        size_t pos = 0;
        size_t lastPos = 0;

        while ((pos = content.find(pattern, pos)) != std::string::npos) {
            bool isWholeWordMatch = true;
            if (m_wholeWord) {
                // Check if the match is a whole word
                bool hasLeftBoundary = (pos == 0 || !std::isalnum(content[pos - 1]));
                bool hasRightBoundary = (pos + pattern.length() == content.length() || 
                                       !std::isalnum(content[pos + pattern.length()]));
                isWholeWordMatch = hasLeftBoundary && hasRightBoundary;
            }

            if (isWholeWordMatch) {
                // Add unmatched part before this match
                if (pos > lastPos) {
                    OutputSubLine unmatched;
                    unmatched.setContent(std::string_view(lineContent.substr(lastPos, pos - lastPos)));
                    sublines.push_back(unmatched);
                }

                // Add matched part with color
                OutputSubLine matchedPart;
                matchedPart.setContent(std::string_view(lineContent.substr(pos, pattern.length())));
                matchedPart.setColor(m_colorString);
                matchedPart.setFilterId(m_filterId);
                matchedPart.setFilterRow(m_filterRow);
                sublines.push_back(matchedPart);

                lastPos = pos + pattern.length();
            }
            pos += pattern.length();
        }

        // Add remaining unmatched part if any
        if (lastPos < lineContent.length()) {
            OutputSubLine unmatched;
            unmatched.setContent(std::string_view(lineContent.substr(lastPos)));
            sublines.push_back(unmatched);
        }
    }

    void FilterData::applyRegex(const std::string_view& lineContent, std::list<OutputSubLine>& sublines){
        try {
            // Prepare regex pattern
            std::string pattern = m_filterPattern;
            if (m_wholeWord) {
                // Add word boundary assertions
                pattern = "\\b" + pattern + "\\b";
            }

            // Set up regex with appropriate flags
            std::regex::flag_type flags = std::regex::ECMAScript;
            if (!m_caseSensitive) {
                flags |= std::regex::icase;
            }
            std::regex re(pattern, flags);

            // Convert string_view to string for regex processing
            std::string lineStr(lineContent);

            // Iterator for regex matches
            std::sregex_iterator begin(lineStr.begin(), lineStr.end(), re);
            std::sregex_iterator end;

            if (begin == end) {
                // No matches found
                return;
            }

            // Found at least one match
            size_t lastPos = 0;

            // Process all matches
            for (std::sregex_iterator it = begin; it != end; ++it) {
                std::smatch match = *it;
                size_t matchPos = match.position();
                size_t matchLen = match.length();

                // Add unmatched part before this match
                if (matchPos > lastPos) {
                    OutputSubLine unmatched;
                    unmatched.setContent(std::string_view(lineContent.substr(lastPos, matchPos - lastPos)));
                    sublines.push_back(unmatched);
                }

                // Add matched part with color
                OutputSubLine matchedPart;
                matchedPart.setContent(match.str());
                matchedPart.setColor(m_colorString);
                matchedPart.setFilterId(m_filterId);
                matchedPart.setFilterRow(m_filterRow);
                sublines.push_back(matchedPart);

                lastPos = matchPos + matchLen;
            }

            // Add remaining unmatched part if any
            if (lastPos < lineContent.length()) {
                OutputSubLine unmatched;
                unmatched.setContent(std::string_view(lineContent.substr(lastPos)));
                sublines.push_back(unmatched);
            }
            else {
                // Add an empty string at the end if the last match is at the end of the line
                //OutputSubLine unmatched;
                //unmatched.setContent("");
                //sublines.push_back(unmatched);
                assert(false);
            }
        }
        catch (const std::regex_error& e) {
            // Handle invalid regex pattern
            Logger::getInstance().error("Invalid regex pattern: " + m_filterPattern + ", error: " + std::string(e.what()));
        }
    }
} // namespace Core 