#include "ColorUtils.h"
#include <cmath>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <stdexcept>

namespace ColorUtils {

RgbColor hexToRgb(const std::string& hex) {
    std::string cleanHex = hex;

    // 移除 # 前缀
    if (cleanHex[0] == '#') {
        cleanHex = cleanHex.substr(1);
    }

    // 验证长度
    if (cleanHex.length() != 6) {
        throw std::invalid_argument("Invalid hex color format: " + hex);
    }

    // 转换为RGB
    try {
        int r = std::stoi(cleanHex.substr(0, 2), nullptr, 16);
        int g = std::stoi(cleanHex.substr(2, 2), nullptr, 16);
        int b = std::stoi(cleanHex.substr(4, 2), nullptr, 16);
        return RgbColor(r, g, b);
    } catch (const std::exception& e) {
        throw std::invalid_argument("Invalid hex color format: " + hex);
    }
}

std::string rgbToHex(const RgbColor& rgb) {
    std::ostringstream oss;
    oss << "#" << std::hex << std::uppercase << std::setfill('0')
        << std::setw(2) << rgb.r
        << std::setw(2) << rgb.g
        << std::setw(2) << rgb.b;
    return oss.str();
}

double colorDistance(const RgbColor& rgb1, const RgbColor& rgb2) {
    double dr = rgb1.r - rgb2.r;
    double dg = rgb1.g - rgb2.g;
    double db = rgb1.b - rgb2.b;
    return std::sqrt(dr * dr + dg * dg + db * db);
}

std::string getContrastColor(const std::string& hex) {
    RgbColor rgb = hexToRgb(hex);

    // 使用亮度公式 Y = 0.2126 R + 0.7152 G + 0.0722 B
    double luma = (0.2126 * rgb.r + 0.7152 * rgb.g + 0.0722 * rgb.b) / 255.0;

    return luma > 0.5 ? "#000000" : "#FFFFFF";
}

PaletteColor findClosestPaletteColor(const RgbColor& targetRgb, const Palette& palette) {
    if (palette.empty()) {
        throw std::invalid_argument("Palette cannot be empty");
    }

    double minDistance = std::numeric_limits<double>::max();
    PaletteColor closestColor = palette[0];

    for (const auto& paletteColor : palette) {
        double distance = colorDistance(targetRgb, paletteColor.rgb);
        if (distance < minDistance) {
            minDistance = distance;
            closestColor = paletteColor;
        }
        if (distance == 0.0) break; // 完全匹配，提前退出
    }

    return closestColor;
}

bool isValidHexColor(const std::string& hex) {
    if (hex.empty()) return false;

    std::string cleanHex = hex;
    if (cleanHex[0] == '#') {
        cleanHex = cleanHex.substr(1);
    }

    if (cleanHex.length() != 6) return false;

    for (char c : cleanHex) {
        if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) {
            return false;
        }
    }

    return true;
}

std::string normalizeHexColor(const std::string& hex) {
    if (!isValidHexColor(hex)) {
        throw std::invalid_argument("Invalid hex color: " + hex);
    }

    std::string cleanHex = hex;
    if (cleanHex[0] == '#') {
        cleanHex = cleanHex.substr(1);
    }

    // 转为大写
    std::transform(cleanHex.begin(), cleanHex.end(), cleanHex.begin(), ::toupper);

    return "#" + cleanHex;
}

} // namespace ColorUtils
