#pragma once

#include <string>
#include <vector>
#include <map>

// RGB颜色结构
struct RgbColor {
    int r, g, b;

    RgbColor() : r(0), g(0), b(0) {}
    RgbColor(int r_, int g_, int b_) : r(r_), g(g_), b(b_) {}

    bool operator==(const RgbColor& other) const {
        return r == other.r && g == other.g && b == other.b;
    }

    bool operator<(const RgbColor& other) const {
        if (r != other.r) return r < other.r;
        if (g != other.g) return g < other.g;
        return b < other.b;
    }
};

// 调色板颜色结构
struct PaletteColor {
    std::string key;    // 色号 (如 "A01", "B12")
    std::string hex;    // 十六进制颜色值 (如 "#FF0000")
    RgbColor rgb;       // RGB值

    PaletteColor() = default;
    PaletteColor(const std::string& k, const std::string& h, const RgbColor& r)
        : key(k), hex(h), rgb(r) {}
};

// 映射像素结构
struct MappedPixel {
    std::string key;      // 色号
    std::string color;    // 十六进制颜色
    bool isExternal;      // 是否为外部/透明区域

    MappedPixel() : isExternal(false) {}
    MappedPixel(const std::string& k, const std::string& c, bool ext = false)
        : key(k), color(c), isExternal(ext) {}
};

// 颜色统计结构
struct ColorCount {
    int count;
    std::string color;

    ColorCount() : count(0) {}
    ColorCount(int c, const std::string& col) : count(c), color(col) {}
};

// Bead颜色结构 (与PaletteColor兼容)
struct BeadColor {
    std::string name;    // 颜色名称
    std::string key;     // 色号 (如 "A01", "B12")
    std::string hex;     // 十六进制颜色值 (如 "#FF0000")
    RgbColor rgb;        // RGB值

    BeadColor() = default;
    BeadColor(const std::string& n, const std::string& k, const std::string& h, const RgbColor& r)
        : name(n), key(k), hex(h), rgb(r) {}
};

// 像素网格结构
struct PixelGrid {
    std::vector<std::vector<MappedPixel>> pixels;
    int width;
    int height;

    PixelGrid() : width(0), height(0) {}
};

// 颜色统计结构
struct ColorStatistics {
    std::map<std::string, int> colorCounts;
    int totalPixels;

    ColorStatistics() : totalPixels(0) {}
};

// 验证结果结构
struct ValidationResult {
    bool isValid;
    std::string errorMessage;

    ValidationResult() : isValid(true) {}
    ValidationResult(bool valid, const std::string& error = "")
        : isValid(valid), errorMessage(error) {}
};

// 像素化模式枚举
enum class PixelationMode {
    DOMINANT,   // 主色模式 (适合卡通图像)
    AVERAGE     // 平均色模式 (适合真实照片)
};

// 渲染模式枚举
enum class RenderMode {
    DPI_BASED,      // DPI模式
    FIXED_WIDTH     // 固定宽度模式
};

// 渲染选项结构
struct RenderOptions {
    bool showGrid = false;
    bool showCoordinates = false;
    bool showColorNames = false;
    bool showStatistics = false;
    std::string backgroundColor = "#FFFFFF";

    RenderMode renderMode = RenderMode::DPI_BASED;
    int dpi = 150;
    int fixedWidth = 0;
};

// 下载选项结构
struct DownloadOptions {
    bool showGrid = false;              // 显示网格线
    int gridInterval = 10;              // 网格间隔
    bool showCoordinates = false;       // 显示坐标轴
    std::string gridLineColor = "#141414";  // 网格线颜色
    std::string outerBorderColor = "";      // 外边框颜色
    bool includeStats = false;          // 包含统计信息
    int dpi = 150;                     // DPI设置
    int fixedWidth = 0;                // 固定宽度 (0表示不使用)
    bool showTransparentLabels = false; // 显示透明标签

    // 新增
    RenderMode renderMode = RenderMode::DPI_BASED;  // 渲染模式
};

// 像素数据结构
struct PixelData {
    std::vector<std::vector<MappedPixel>> mappedData;
    int width;
    int height;

    PixelData() : width(0), height(0) {}
};

// 图像生成参数结构
struct GenerateImageParams {
    std::string title;
    PixelData pixelData;
    RenderMode renderMode;
    DownloadOptions options;
};

// 全局类型定义
using ColorCountMap = std::map<std::string, ColorCount>;
using Palette = std::vector<PaletteColor>;
