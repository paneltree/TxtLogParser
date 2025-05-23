#include "SearchData.h"
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

    SearchData::SearchData(){
        m_searchId = -1;
        m_searchRow = -1;
        m_colorString = "";
        Logger::getInstance().debug("Created new search with color: " + m_colorString);
    }

    SearchData::SearchData( int32_t id,
                            int32_t row,
                            const std::string& pattern,
                            bool caseSensitive,
                            bool wholeWord,
                            bool regex,
                            bool enabled,
                            const std::string& color) 
        :   m_searchId(id),
            m_searchRow(row),
            m_searchPattern(pattern),
            m_caseSensitive(caseSensitive),
            m_wholeWord(wholeWord),
            m_regex(regex),
            m_enabled(enabled) {
            m_colorString = color;
    }

    bool SearchData::saveToJson(json& j) const
    {
        j["id"] = m_searchId;
        j["row"] = m_searchRow;
        j["pattern"] = m_searchPattern;
        j["caseSensitive"] = m_caseSensitive;
        j["wholeWord"] = m_wholeWord;
        j["regex"] = m_regex;
        j["enabled"] = m_enabled;
        j["color"] = m_colorString;
        return true;

    }
    bool SearchData::loadFromJson(const json& j)
    {
        m_searchId = j.value("id", -1);
        m_searchRow = j.value("row", -1);
        m_searchPattern = j.value("pattern", "");
        m_caseSensitive = j.value("caseSensitive", false);
        m_wholeWord = j.value("wholeWord", false);
        m_regex = j.value("regex", false);
        m_enabled = j.value("enabled", true);
        m_colorString = j.value("color", "");
        return true;        
    }

    void SearchData::update(const SearchData& search, bool* pChanged){
        bool changed = false;
        assert(m_searchId == search.m_searchId);
        assert(m_searchRow == search.m_searchRow);  
        if(m_searchPattern != search.m_searchPattern){
            m_searchPattern = search.m_searchPattern;
            changed = true;
        }
        if(m_caseSensitive != search.m_caseSensitive){
            m_caseSensitive = search.m_caseSensitive;
            changed = true;
        }
        if(m_wholeWord != search.m_wholeWord){
            m_wholeWord = search.m_wholeWord;
            changed = true;
        }
        if(m_regex != search.m_regex){
            m_regex = search.m_regex;
            changed = true;
        }
        if(m_enabled != search.m_enabled){
            m_enabled = search.m_enabled;   
            changed = true;
        }
        if(m_colorString != search.m_colorString){
            m_colorString = search.m_colorString;
            changed = true;
        }   
        if(pChanged != nullptr){
            *pChanged = changed;
        }
    }

    void SearchData::apply(const std::string_view& lineContent, std::list<OutputSubLine>& sublines){
        assert(!m_searchPattern.empty());
        if(m_enabled){
            if(!m_regex){
                applyNonRegex(lineContent, sublines);
            }
            else{
                applyRegex(lineContent, sublines);
            }
        }
    }

    void SearchData::applyNonRegex(const std::string_view& lineContent, std::list<OutputSubLine>& sublines){
        // Convert pattern and content to lowercase if case insensitive
        std::string pattern = m_searchPattern;
        std::string content(lineContent);
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
                    unmatched.setContent(lineContent.substr(lastPos, pos - lastPos));
                    sublines.push_back(unmatched);
                }

                // Add matched part with color
                OutputSubLine matchedPart;
                matchedPart.setContent(lineContent.substr(pos, pattern.length()));
                matchedPart.setColor(m_colorString);
                matchedPart.setSearchId(m_searchId);
                matchedPart.setSearchRow(m_searchRow);
                sublines.push_back(matchedPart);

                lastPos = pos + pattern.length();
            }
            pos += pattern.length();
        }

        // Add remaining unmatched part if any
        if (lastPos < lineContent.length()) {
            OutputSubLine unmatched;
            unmatched.setContent(lineContent.substr(lastPos));
            sublines.push_back(unmatched);
        }
    }

    void SearchData::applyRegex(const std::string_view& lineContent, std::list<OutputSubLine>& sublines){
        try {
            // Convert string_view to string for regex operations
            std::string contentStr(lineContent);
            
            // Prepare regex pattern
            std::string pattern = m_searchPattern;
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

            // Iterator for regex matches
            std::sregex_iterator begin(contentStr.begin(), contentStr.end(), re);
            std::sregex_iterator end;

            if (begin == end) {
                // No matches found
                // Add the entire line as unmatched
                OutputSubLine unmatched;
                unmatched.setContent(lineContent);
                sublines.push_back(unmatched);
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
                    unmatched.setContent(lineContent.substr(lastPos, matchPos - lastPos));
                    sublines.push_back(unmatched);
                }

                // Add matched part with color
                OutputSubLine matchedPart;
                matchedPart.setContent(lineContent.substr(matchPos, matchLen));
                matchedPart.setColor(m_colorString);
                matchedPart.setSearchId(m_searchId);
                matchedPart.setSearchRow(m_searchRow);
                sublines.push_back(matchedPart);

                lastPos = matchPos + matchLen;
            }

            // Add remaining unmatched part if any
            if (lastPos < lineContent.length()) {
                OutputSubLine unmatched;
                unmatched.setContent(lineContent.substr(lastPos));
                sublines.push_back(unmatched);
            }
            else {
                // Add an empty string at the end if the last match is at the end of the line
                //OutputSubLine unmatched;
                //unmatched.setContent("");
                //sublines.push_back(unmatched);
            }
        }
        catch (const std::regex_error& e) {
            // Handle invalid regex pattern
            Logger::getInstance().error("Invalid regex pattern: " + m_searchPattern + ", error: " + std::string(e.what()));
        }
    }
} // namespace Core 