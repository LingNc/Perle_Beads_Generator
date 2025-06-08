#include "ColorPalette.h"
#include "ColorUtils.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <regex>

ColorPalette::ColorPalette() {
    loadDefaultPalette();
}

ColorPalette::~ColorPalette() = default;

bool ColorPalette::loadDefaultPalette() {
    palette_.clear();

    // 内置的基础MARD调色板 - 基于常用颜色
    // 这里先添加一些基础颜色，实际项目中应该从文件加载完整调色板
    std::vector<std::pair<std::string, std::string>> defaultColors = {
        {"A01", "#FF0000"}, // 红色
        {"A02", "#00FF00"}, // 绿色
        {"A03", "#0000FF"}, // 蓝色
        {"A04", "#FFFF00"}, // 黄色
        {"A05", "#FF00FF"}, // 洋红
        {"A06", "#00FFFF"}, // 青色
        {"A07", "#FFFFFF"}, // 白色
        {"A08", "#000000"}, // 黑色
        {"A09", "#808080"}, // 灰色
        {"A10", "#FFA500"}, // 橙色
        {"B01", "#800080"}, // 紫色
        {"B02", "#008000"}, // 深绿色
        {"B03", "#000080"}, // 深蓝色
        {"B04", "#800000"}, // 深红色
        {"B05", "#808000"}, // 橄榄色
        {"B06", "#008080"}, // 青绿色
        {"B07", "#C0C0C0"}, // 银色
        {"B08", "#FFE4B5"}, // 桃色
        {"B09", "#DDA0DD"}, // 李子色
        {"B10", "#F0E68C"}, // 卡其色
        {"C01", "#FF69B4"}, // 热粉色
        {"C02", "#FF6347"}, // 番茄红
        {"C03", "#4169E1"}, // 皇室蓝
        {"C04", "#32CD32"}, // 酸橙绿
        {"C05", "#FFD700"}, // 金色
        {"T01", "#FFFFFF"}, // 透明色 (用白色表示)
        {"H01", "#F8F8FF"}, // 幽灵白
        {"H02", "#FFFAFA"}, // 雪白
        {"P12", "#B0C4DE"}  // 浅钢蓝
    };

    for (const auto& [key, hex] : defaultColors) {
        addColor(key, hex);
    }

    std::cout << "Loaded default palette with " << palette_.size() << " colors" << std::endl;
    return !palette_.empty();
}

bool ColorPalette::loadPaletteFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open palette file: " << filename << std::endl;
        return false;
    }

    palette_.clear();

    // 检查文件扩展名来决定解析方式
    std::string extension = filename.substr(filename.find_last_of(".") + 1);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    bool success = false;
    if (extension == "json") {
        success = parseJsonPalette(file);
    } else {
        success = parseSimplePalette(file);
    }

    file.close();

    if (palette_.empty()) {
        std::cerr << "No valid colors loaded from palette file" << std::endl;
        return false;
    }

    std::cout << "Loaded palette from " << filename << " with " << palette_.size() << " colors" << std::endl;
    return true;
}

PaletteColor* ColorPalette::findColorByKey(const std::string& key) {
    auto it = std::find_if(palette_.begin(), palette_.end(),
        [&key](const PaletteColor& color) {
            return color.key == key;
        });

    return (it != palette_.end()) ? &(*it) : nullptr;
}

PaletteColor* ColorPalette::findColorByHex(const std::string& hex) {
    std::string normalizedHex = ColorUtils::normalizeHexColor(hex);

    auto it = std::find_if(palette_.begin(), palette_.end(),
        [&normalizedHex](const PaletteColor& color) {
            return color.hex == normalizedHex;
        });

    return (it != palette_.end()) ? &(*it) : nullptr;
}

PaletteColor ColorPalette::findClosestColor(const RgbColor& targetRgb) const {
    return ColorUtils::findClosestPaletteColor(targetRgb, palette_);
}

PaletteColor ColorPalette::getTransparentFallbackColor() const {
    // 按优先级查找透明色备用颜色
    std::vector<std::string> priorityKeys = {"T01", "H02", "H01", "P12"};

    for (const std::string& key : priorityKeys) {
        auto it = std::find_if(palette_.begin(), palette_.end(),
            [&key](const PaletteColor& color) {
                return color.key == key;
            });

        if (it != palette_.end()) {
            std::cout << "Found transparent fallback color: " << key
                      << " (" << it->hex << ")" << std::endl;
            return *it;
        }
    }

    // 如果没找到优先级颜色，使用调色板第一个颜色
    if (!palette_.empty()) {
        std::cout << "Using first palette color as fallback: "
                  << palette_[0].key << " (" << palette_[0].hex << ")" << std::endl;
        return palette_[0];
    }

    throw std::runtime_error("Palette is empty, cannot get fallback color");
}

void ColorPalette::printPaletteInfo() const {
    std::cout << "=== Palette Information ===" << std::endl;
    std::cout << "Total colors: " << palette_.size() << std::endl;

    for (const auto& color : palette_) {
        std::cout << color.key << " " << color.hex
                  << " RGB(" << color.rgb.r << "," << color.rgb.g << "," << color.rgb.b << ")"
                  << std::endl;
    }
    std::cout << "=========================" << std::endl;
}

void ColorPalette::addColor(const std::string& key, const std::string& hex) {
    if (key.empty()) {
        throw std::invalid_argument("Color key cannot be empty");
    }

    if (!ColorUtils::isValidHexColor(hex)) {
        throw std::invalid_argument("Invalid hex color: " + hex);
    }

    std::string normalizedHex = ColorUtils::normalizeHexColor(hex);
    RgbColor rgb = ColorUtils::hexToRgb(normalizedHex);

    palette_.emplace_back(key, normalizedHex, rgb);
}

bool compareColorKeys(const std::string& a, const std::string& b) {
    // 解析色号格式: 字母前缀 + 数字 (如 A01, B12)
    std::regex pattern(R"(^([A-Z]+)(\d+)$)");
    std::smatch matchA, matchB;

    bool validA = std::regex_match(a, matchA, pattern);
    bool validB = std::regex_match(b, matchB, pattern);

    if (validA && validB) {
        // 先按字母前缀排序
        std::string prefixA = matchA[1].str();
        std::string prefixB = matchB[1].str();

        if (prefixA != prefixB) {
            return prefixA < prefixB;
        }

        // 然后按数字排序
        int numA = std::stoi(matchA[2].str());
        int numB = std::stoi(matchB[2].str());
        return numA < numB;
    }

    // 如果不符合标准格式，使用字符串比较
    return a < b;
}

bool ColorPalette::parseJsonPalette(std::ifstream& file) {
    // 简单的JSON解析器，专门处理包含selectedHexValues数组的格式
    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());

    // 查找selectedHexValues数组
    size_t arrayStart = content.find("\"selectedHexValues\"");
    if (arrayStart == std::string::npos) {
        std::cerr << "JSON palette must contain 'selectedHexValues' array" << std::endl;
        return false;
    }

    // 找到数组开始的方括号
    size_t bracketStart = content.find("[", arrayStart);
    if (bracketStart == std::string::npos) {
        std::cerr << "Invalid JSON format: cannot find array start" << std::endl;
        return false;
    }

    // 找到数组结束的方括号
    size_t bracketEnd = content.find("]", bracketStart);
    if (bracketEnd == std::string::npos) {
        std::cerr << "Invalid JSON format: cannot find array end" << std::endl;
        return false;
    }

    // 提取数组内容
    std::string arrayContent = content.substr(bracketStart + 1,
                                            bracketEnd - bracketStart - 1);

    // 解析hex颜色值
    std::istringstream stream(arrayContent);
    std::string token;
    int colorIndex = 1;

    while (std::getline(stream, token, ',')) {
        // 清理token（移除引号、空格等）
        token.erase(std::remove_if(token.begin(), token.end(),
                   [](char c) { return c == '"' || c == ' ' || c == '\n' || c == '\r' || c == '\t'; }),
                   token.end());

        if (!token.empty() && token[0] == '#') {
            try {
                std::string key = "C" + std::to_string(colorIndex++);
                addColor(key, token);
            } catch (const std::exception& e) {
                std::cerr << "Error adding color " << token << ": " << e.what() << std::endl;
            }
        }
    }

    return !palette_.empty();
}

bool ColorPalette::parseSimplePalette(std::ifstream& file) {
    std::string line;
    int lineNumber = 0;

    while (std::getline(file, line)) {
        lineNumber++;

        // 跳过注释和空行
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // 解析格式: KEY #HEXCOLOR (例如: A01 #FF0000)
        std::istringstream iss(line);
        std::string key, hex;

        if (iss >> key >> hex) {
            try {
                addColor(key, hex);
            } catch (const std::exception& e) {
                std::cerr << "Error parsing line " << lineNumber << ": " << e.what() << std::endl;
            }
        } else {
            std::cerr << "Invalid format at line " << lineNumber << ": " << line << std::endl;
        }
    }

    return !palette_.empty();
}
