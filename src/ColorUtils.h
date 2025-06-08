#pragma once

#include "Types.h"
#include <string>

namespace ColorUtils {
    // 将十六进制颜色转换为RGB
    RgbColor hexToRgb(const std::string& hex);

    // 将RGB转换为十六进制颜色
    std::string rgbToHex(const RgbColor& rgb);

    // 计算两个RGB颜色之间的距离
    double colorDistance(const RgbColor& rgb1, const RgbColor& rgb2);

    // 获取与背景色形成对比的文字颜色
    std::string getContrastColor(const std::string& hex);

    // 查找调色板中最接近的颜色
    PaletteColor findClosestPaletteColor(const RgbColor& targetRgb, const Palette& palette);

    // 验证十六进制颜色格式
    bool isValidHexColor(const std::string& hex);

    // 标准化十六进制颜色值 (转大写, 添加#前缀等)
    std::string normalizeHexColor(const std::string& hex);
}
