#ifndef CORE_FILTERCOLORMANAGER_H
#define CORE_FILTERCOLORMANAGER_H

#include <string>
#include <vector>
#include <memory>
#include <random>
#include <ctime>
#include <map>
#include "ColorData.h"

namespace Core {

/**
 * @brief Pure C++ class for managing filter colors
 * 
 * This class handles the generation and management of filter colors
 * in a way that ensures good contrast and readability.
 */
class FilterSearchColorManager {
public:
    FilterSearchColorManager();
    std::string getNextColor();
    void pushColor(const std::string& color);
    void popColor(const std::string& color);
private:
    ColorDataManager m_colorDataManager;
    std::map<size_t/*index*/, std::string/*color*/> m_predefinedIndexToColor;
    std::map<std::string/*color*/, size_t/*index*/> m_predefinedColorToIndex;
    std::set<size_t> m_usedPredefinedColors;
    std::set<size_t> m_unusedPredefinedColors;
    std::set<std::string> m_usedCustomColors;
    std::set<std::string> m_unusedCustomColors;
};

} // namespace Core

#endif // CORE_FILTERCOLORMANAGER_H 