#pragma once

#include "Types.h"
#include <string>
#include <vector>

class ColorPalette {
public:
    ColorPalette();
    ~ColorPalette();

    // 加载默认调色板 (基于MARD色号系统)
    bool loadDefaultPalette();

    // 从文件加载自定义调色板
    bool loadPaletteFromFile(const std::string& filename);

    // 获取调色板
    const Palette& getPalette() const { return palette_; }

    // 获取调色板大小
    size_t size() const { return palette_.size(); }

    // 根据色号查找颜色
    PaletteColor* findColorByKey(const std::string& key);

    // 根据hex值查找颜色
    PaletteColor* findColorByHex(const std::string& hex);

    // 查找最接近的调色板颜色
    PaletteColor findClosestColor(const RgbColor& targetRgb) const;

    // 验证调色板是否有效
    bool isValid() const { return !palette_.empty(); }

    // 获取透明/背景色备用颜色
    PaletteColor getTransparentFallbackColor() const;

    // 打印调色板信息 (用于调试)
    void printPaletteInfo() const;

private:
    Palette palette_;

    // 解析颜色映射文件
    bool parseColorMapping();

    // 解析JSON格式调色板
    bool parseJsonPalette(std::ifstream& file);

    // 解析简单格式调色板 (KEY #HEX)
    bool parseSimplePalette(std::ifstream& file);

    // 添加颜色到调色板
    void addColor(const std::string& key, const std::string& hex);
};

// 用于颜色键排序的比较函数
bool compareColorKeys(const std::string& a, const std::string& b);
