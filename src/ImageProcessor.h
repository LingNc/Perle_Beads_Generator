#pragma once

#include "Types.h"
#include "ColorPalette.h"
#include <string>
#include <vector>

class ImageProcessor {
public:
    ImageProcessor();
    ~ImageProcessor();

    // 加载图像文件
    bool loadImage(const std::string& filename);

    // 设置调色板
    void setPalette(const ColorPalette& palette);

    // 设置像素化模式
    void setPixelationMode(PixelationMode mode) { pixelationMode_ = mode; }

    // 计算像素网格
    PixelData calculatePixelGrid(int gridWidth, int gridHeight);

    // 获取原始图像尺寸
    void getImageSize(int& width, int& height) const;

    // 验证图像是否已加载
    bool isImageLoaded() const { return imageData_ != nullptr; }

    // 计算颜色统计
    ColorCountMap calculateColorCounts(const PixelData& pixelData);

    // 过滤颜色统计（排除透明色）
    ColorCountMap filterColorCountsForBeadUsage(const ColorCountMap& colorCounts, bool excludeTransparent = true);

private:
    // 图像数据
    unsigned char* imageData_;
    int imageWidth_;
    int imageHeight_;

    // 处理参数
    ColorPalette palette_;
    PixelationMode pixelationMode_;

    // 计算单元格的代表色
    RgbColor calculateCellRepresentativeColor(int startX, int startY, int cellWidth, int cellHeight);

    // 计算单元格的主色（dominant模式）
    RgbColor calculateDominantColor(int startX, int startY, int cellWidth, int cellHeight);

    // 计算单元格的平均色（average模式）
    RgbColor calculateAverageColor(int startX, int startY, int cellWidth, int cellHeight);

    // 获取像素颜色
    RgbColor getPixelColor(int x, int y) const;

    // 释放图像数据
    void freeImageData();
};
