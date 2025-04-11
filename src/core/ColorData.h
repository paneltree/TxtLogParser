#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <map>
#include <set>
namespace Core {
struct ColorData {
    int32_t m_red;
    int32_t m_green;
    int32_t m_blue;

    ColorData() : m_red(0), m_green(0), m_blue(0) {}
    ColorData(const std::string& hex) {
        hexToRgb(hex, m_red, m_green, m_blue);
    }
    ColorData(int32_t red, int32_t green, int32_t blue) : m_red(red), m_green(green), m_blue(blue) {}
    
    // Calculate luminance using the formula Y = 0.299R + 0.587G + 0.114B
    double getLuminance() const {
        return 0.299 * m_red + 0.587 * m_green + 0.114 * m_blue;
    }
    
    // Calculate contrast ratio against a background color
    double getContrastRatio(const ColorData& background) const;

    bool operator==(const ColorData& other) const {
        return m_red == other.m_red && m_green == other.m_green && m_blue == other.m_blue;
    }
    // Add this to the ColorData class declaration
    bool operator<(const ColorData& other) const {
        // Lexicographical comparison of RGB values
        if (m_red != other.m_red) return m_red < other.m_red;
        if (m_green != other.m_green) return m_green < other.m_green;
        return m_blue < other.m_blue;
    }
    std::string toString() const;
    static std::string rgbToHex(int r, int g, int b);
    static void hexToRgb(const std::string& hex, int& r, int& g, int& b);
    static bool isColorValid(const std::string& hex);
    static double calculateLuminance(int r, int g, int b);
};


class ColorDataManager {
    public:
        ColorDataManager();
        std::vector<std::string> getAllColors() const;
};

} // namespace Core
