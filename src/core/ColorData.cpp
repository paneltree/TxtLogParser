#include "ColorData.h"
#include <cmath>
#include <algorithm>
#include <random>
#include <sstream>
#include <iomanip>
namespace Core {

namespace{
const char* g_colors_1[] = {
    "#F44336",  // Red
    "#DFEE15", // Pink
    "#37B027", // Purple
    "#187DCA", // Deep Purple
    "#CA692D", // Indigo
    "#2195F3", // Blue
    "#03F4D8", // Light Blue
    "#D400C9", // Cyan
    "#002396", // Teal
    "#37F73D", // Green
    "#67AE4A", // Light Green
    "#39C6DC", // Lime
    "#FFEB3B", // Yellow
    "#FFC107", // Amber
    "#2600FF", // Orange
    "#FF5722", // Deep Orange
    "#E22ED3", // Brown
    "#67E1AC", // Grey
    "#C3F748", // Blue Grey
    "#2D5E71", // Black
};
}

std::string ColorData::toString() const {
    return ColorData::rgbToHex(m_red, m_green, m_blue);
}

std::string ColorData::rgbToHex(int r, int g, int b) {
    std::stringstream ss;
    ss << "#"
       << std::hex << std::setw(2) << std::setfill('0') << r
       << std::hex << std::setw(2) << std::setfill('0') << g
       << std::hex << std::setw(2) << std::setfill('0') << b;
    return ss.str();
}

void ColorData::hexToRgb(const std::string& hex, int& r, int& g, int& b) {
    // Remove '#' if present
    std::string cleanHex = hex;
    if (cleanHex[0] == '#') {
        cleanHex = cleanHex.substr(1);
    }
    
    // Convert hex to RGB
    std::stringstream ss;
    ss << std::hex << cleanHex;
    unsigned int color;
    ss >> color;
    
    r = (color >> 16) & 0xFF;
    g = (color >> 8) & 0xFF;
    b = color & 0xFF;
}

bool ColorData::isColorValid(const std::string& hex) {
    if (hex.empty() || (hex[0] != '#' && hex.length() != 6) || (hex[0] == '#' && hex.length() != 7)) {
        return false;
    }
    
    int r, g, b;
    hexToRgb(hex, r, g, b);
    
    double luminance = calculateLuminance(r, g, b);
    return luminance >= 0.2 && luminance <= 0.8;
}

double ColorData::calculateLuminance(int r, int g, int b) {
    // Convert RGB to relative luminance using the formula from WCAG 2.0
    double rr = r / 255.0;
    double gg = g / 255.0;
    double bb = b / 255.0;
    
    rr = (rr <= 0.03928) ? rr / 12.92 : std::pow((rr + 0.055) / 1.055, 2.4);
    gg = (gg <= 0.03928) ? gg / 12.92 : std::pow((gg + 0.055) / 1.055, 2.4);
    bb = (bb <= 0.03928) ? bb / 12.92 : std::pow((bb + 0.055) / 1.055, 2.4);
    
    return 0.2126 * rr + 0.7152 * gg + 0.0722 * bb;
}

// Calculate contrast ratio based on WCAG formula
// Contrast ratio = (L1 + 0.05) / (L2 + 0.05), where L1 is the lighter color and L2 is the darker one
double ColorData::getContrastRatio(const ColorData& background) const {
    double foregroundLuminance = getLuminance() / 255.0;
    double backgroundLuminance = background.getLuminance() / 255.0;
    
    double lighter = std::max(foregroundLuminance, backgroundLuminance);
    double darker = std::min(foregroundLuminance, backgroundLuminance);
    
    return (lighter + 0.05) / (darker + 0.05);
}

ColorDataManager::ColorDataManager() {
}



std::vector<std::string> ColorDataManager::getAllColors() const {
    std::vector<std::string> allColors;
    for (const auto& color : g_colors_1) {
        allColors.push_back(color);
    }
    return allColors;
}
} // namespace Core
