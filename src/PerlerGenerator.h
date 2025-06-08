#pragma once

#include "Types.h"
#include "ColorPalette.h"
#include "ImageProcessor.h"
#include <string>
#include <cairo/cairo.h>

class PerlerGenerator {
public:
    PerlerGenerator();
    ~PerlerGenerator();

    // 设置调色板
    void setPalette(const ColorPalette& palette);

    // 从图像生成拼豆图纸
    bool generateFromImage(const std::string& inputPath, const GenerateImageParams& params);

    // 保存生成的图纸
    bool saveImage(const std::string& outputPath);

    // 保存颜色统计
    bool saveColorStats(const std::string& statsPath, const std::string& title = "");

    // 获取生成的像素数据 (用于外部访问)
    const PixelData& getPixelData() const { return pixelData_; }

    // 获取颜色统计
    const ColorCountMap& getColorCounts() const { return colorCounts_; }

    // 获取总珠子数量
    int getTotalBeadCount() const { return totalBeadCount_; }

private:
    // 成员变量
    ColorPalette palette_;
    ImageProcessor imageProcessor_;
    PixelData pixelData_;
    ColorCountMap colorCounts_;
    int totalBeadCount_;

    // Cairo相关
    cairo_surface_t* surface_;
    cairo_t* cr_;
    int canvasWidth_;
    int canvasHeight_;

    // 渲染参数
    GenerateImageParams params_;

    // 初始化Cairo画布
    bool initCanvas(const GenerateImageParams& params);

    // 清理Cairo资源
    void cleanupCanvas();

    // 计算画布尺寸
    void calculateCanvasSize(const GenerateImageParams& params, int& width, int& height);

    // 渲染图纸
    void renderPattern();

    // 渲染标题
    void renderTitle();

    // 渲染坐标轴
    void renderCoordinates();

    // 渲染网格单元格
    void renderCells();

    // 渲染网格线
    void renderGridLines();

    // 渲染外边框
    void renderBorder();

    // 渲染统计信息
    void renderStatistics();

    // 辅助函数
    void setColor(const std::string& hexColor);
    void drawText(const std::string& text, double x, double y, double fontSize, const std::string& color = "#000000");
    void drawCenteredText(const std::string& text, double x, double y, double fontSize, const std::string& color = "#000000");
    double getTextWidth(const std::string& text, double fontSize);

    // 获取DPI缩放比例
    double getDpiScale() const;

    // 获取单元格大小
    double getCellSize() const;
};
