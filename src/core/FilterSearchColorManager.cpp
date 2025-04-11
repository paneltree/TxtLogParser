#include "FilterSearchColorManager.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include "Logger.h"
namespace Core {

FilterSearchColorManager::FilterSearchColorManager() {
    m_colorDataManager = ColorDataManager();
    std::vector<std::string> predefinedColors = m_colorDataManager.getAllColors();
    m_usedPredefinedColors = std::set<size_t>();
    m_unusedPredefinedColors = std::set<size_t>();
    m_usedCustomColors = std::set<std::string>();
    m_unusedCustomColors = std::set<std::string>();
    for(size_t i = 0; i < predefinedColors.size(); i++) {
        m_predefinedIndexToColor[i] = predefinedColors[i];
        m_predefinedColorToIndex[predefinedColors[i]] = i;
        m_unusedPredefinedColors.insert(i);
    }
}

std::string FilterSearchColorManager::getNextColor() {
    if(!m_unusedCustomColors.empty()) {
        std::string color = *m_unusedCustomColors.begin();
        return color;
    }
    if(!m_unusedPredefinedColors.empty()) {
        size_t index = *m_unusedPredefinedColors.begin();
        return m_predefinedIndexToColor[index];
    }
    //create a new color not in the m_usedPredefinedColors and m_usedCustomColors
    std::string color = "#000000";
    return color;
}

void FilterSearchColorManager::pushColor(const std::string& color) {
    //to uppercase
    std::string upperColor;
    upperColor.resize(color.size());
    std::transform(color.begin(), color.end(), upperColor.begin(), ::toupper);
    if(upperColor == "#000000") {
        return;
    }
    auto it = m_predefinedColorToIndex.find(upperColor);
    if(it != m_predefinedColorToIndex.end()) {
        m_unusedPredefinedColors.insert(it->second);
        m_usedPredefinedColors.erase(it->second);
        return;
    }
    m_unusedCustomColors.insert(upperColor);
    m_usedCustomColors.erase(upperColor);
}

void FilterSearchColorManager::popColor(const std::string& color) {
    std::string upperColor;
    upperColor.resize(color.size());
    std::transform(color.begin(), color.end(), upperColor.begin(), ::toupper);
    auto it = m_predefinedColorToIndex.find(upperColor);
    if(it != m_predefinedColorToIndex.end()) {
        m_unusedPredefinedColors.erase(it->second);
        m_usedPredefinedColors.insert(it->second);
        return;
    }
    m_unusedCustomColors.erase(upperColor);
    m_usedCustomColors.insert(upperColor);
}
}