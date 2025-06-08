#include "PerlerGenerator.h"
#include "ColorUtils.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <iomanip>
#include <ctime>
#include <algorithm>

PerlerGenerator::PerlerGenerator()
    : totalBeadCount_(0), surface_(nullptr), cr_(nullptr), canvasWidth_(0), canvasHeight_(0) {
}

PerlerGenerator::~PerlerGenerator() {
    cleanupCanvas();
}

void PerlerGenerator::setPalette(const ColorPalette& palette) {
    palette_ = palette;
    imageProcessor_.setPalette(palette);
}

bool PerlerGenerator::generateFromImage(const std::string& inputPath, const GenerateImageParams& params) {
    params_ = params;

    // 加载图像
    if (!imageProcessor_.loadImage(inputPath)) {
        std::cerr << "Failed to load image: " << inputPath << std::endl;
        return false;
    }

    // 获取图像尺寸
    int imageWidth, imageHeight;
    imageProcessor_.getImageSize(imageWidth, imageHeight);

    // 如果没有指定网格尺寸，使用图像原始尺寸
    int gridWidth = params.pixelData.width > 0 ? params.pixelData.width : imageWidth;
    int gridHeight = params.pixelData.height > 0 ? params.pixelData.height : imageHeight;

    // 生成像素网格
    pixelData_ = imageProcessor_.calculatePixelGrid(gridWidth, gridHeight);

    // 计算颜色统计
    ColorCountMap allCounts = imageProcessor_.calculateColorCounts(pixelData_);
    colorCounts_ = imageProcessor_.filterColorCountsForBeadUsage(allCounts, !params.options.showTransparentLabels);

    // 计算总珠子数量
    totalBeadCount_ = 0;
    for (const auto& [key, colorData] : colorCounts_) {
        totalBeadCount_ += colorData.count;
    }

    // 初始化画布并渲染
    if (!initCanvas(params)) {
        return false;
    }

    renderPattern();

    std::cout << "Generated pattern with " << colorCounts_.size()
              << " colors, total " << totalBeadCount_ << " beads" << std::endl;

    return true;
}

bool PerlerGenerator::saveImage(const std::string& outputPath) {
    if (!surface_) {
        std::cerr << "No image to save" << std::endl;
        return false;
    }

    cairo_status_t status = cairo_surface_write_to_png(surface_, outputPath.c_str());

    if (status != CAIRO_STATUS_SUCCESS) {
        std::cerr << "Failed to save image: " << cairo_status_to_string(status) << std::endl;
        return false;
    }

    std::cout << "Saved image to: " << outputPath << std::endl;
    return true;
}

bool PerlerGenerator::saveColorStats(const std::string& statsPath, const std::string& title) {
    std::ofstream file(statsPath);
    if (!file.is_open()) {
        std::cerr << "Failed to create stats file: " << statsPath << std::endl;
        return false;
    }

    // 写入头部信息
    file << "# Perler Bead Color Statistics\n";

    // 添加时间戳
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    file << "# Generated on: " << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "\n";

    if (!title.empty()) {
        file << "# Title: " << title << "\n";
    }

    file << "# Grid Size: " << pixelData_.width << "x" << pixelData_.height << "\n";
    file << "# Total Beads: " << totalBeadCount_ << "\n";
    file << "\n";

    // 按色号排序输出
    std::vector<std::pair<std::string, ColorCount>> sortedColors;
    for (const auto& [key, colorData] : colorCounts_) {
        sortedColors.emplace_back(key, colorData);
    }

    std::sort(sortedColors.begin(), sortedColors.end(),
        [](const auto& a, const auto& b) {
            return compareColorKeys(a.first, b.first);
        });

    // 写入颜色统计
    for (const auto& [key, colorData] : sortedColors) {
        file << key << " " << colorData.color << " " << colorData.count << "\n";
    }

    file << "\nTOTAL: " << totalBeadCount_ << "\n";

    file.close();
    std::cout << "Saved color statistics to: " << statsPath << std::endl;
    return true;
}

bool PerlerGenerator::initCanvas(const GenerateImageParams& params) {
    cleanupCanvas();

    calculateCanvasSize(params, canvasWidth_, canvasHeight_);

    surface_ = cairo_image_surface_create(CAIRO_FORMAT_RGB24, canvasWidth_, canvasHeight_);
    if (cairo_surface_status(surface_) != CAIRO_STATUS_SUCCESS) {
        std::cerr << "Failed to create Cairo surface" << std::endl;
        return false;
    }

    cr_ = cairo_create(surface_);
    if (cairo_status(cr_) != CAIRO_STATUS_SUCCESS) {
        std::cerr << "Failed to create Cairo context" << std::endl;
        return false;
    }

    // 设置白色背景
    cairo_set_source_rgb(cr_, 1.0, 1.0, 1.0);
    cairo_paint(cr_);

    return true;
}

void PerlerGenerator::cleanupCanvas() {
    if (cr_) {
        cairo_destroy(cr_);
        cr_ = nullptr;
    }
    if (surface_) {
        cairo_surface_destroy(surface_);
        surface_ = nullptr;
    }
}

void PerlerGenerator::calculateCanvasSize(const GenerateImageParams& params, int& width, int& height) {
    double dpiScale = getDpiScale();
    double cellSize = getCellSize();

    // 计算网格尺寸
    double gridWidth = pixelData_.width * cellSize;
    double gridHeight = pixelData_.height * cellSize;

    // 计算坐标轴空间
    double axisLabelSize = params.options.showCoordinates ? std::max(30.0 * dpiScale, cellSize) : 0.0;

    // 计算边距
    double extraLeftMargin = (params.options.showCoordinates ? std::max(20.0 * dpiScale, 40.0) : 0.0) + 15.0 * dpiScale;
    double extraRightMargin = (params.options.showCoordinates ? std::max(20.0 * dpiScale, 40.0) : 0.0) + 35.0 * dpiScale;
    double extraTopMargin = params.options.showCoordinates ? std::max(15.0 * dpiScale, 20.0) : 20.0 * dpiScale;
    double extraBottomMargin = 20.0 * dpiScale;

    // 计算标题高度
    double titleBarHeight = !params.title.empty() ? 80.0 * dpiScale : 0.0;

    // 计算统计区域高度
    double statsHeight = 0.0;
    if (params.options.includeStats && !colorCounts_.empty()) {
        double statsFontSize = 13.0 * dpiScale;
        double statsRowHeight = std::max(24.0 * dpiScale, 28.0 * dpiScale) + 16.0 * dpiScale;
        int numColumns = std::max(1, static_cast<int>(gridWidth / 150.0)); // 估算列数
        int numRows = static_cast<int>(std::ceil(static_cast<double>(colorCounts_.size()) / numColumns));

        statsHeight = 2.0 * (statsFontSize + 2.0 * dpiScale) + // 标题高度
                     numRows * statsRowHeight +                  // 内容高度
                     30.0 * dpiScale +                          // 总计高度
                     40.0 * dpiScale +                          // 内边距
                     24.0 * dpiScale;                           // 顶部边距
    }

    // 计算最终画布尺寸，添加更多的边距以确保居中和美观
    double horizontalPadding = 80.0 * dpiScale;  // 增加水平边距以确保居中
    double verticalPadding = 60.0 * dpiScale;    // 增加垂直边距以确保居中

    width = static_cast<int>(gridWidth + axisLabelSize + extraLeftMargin + extraRightMargin + horizontalPadding);
    height = static_cast<int>(titleBarHeight + gridHeight + axisLabelSize + statsHeight + extraTopMargin + extraBottomMargin + verticalPadding);

    std::cout << "Canvas size: " << width << "x" << height << std::endl;
}

void PerlerGenerator::renderPattern() {
    renderTitle();
    renderCoordinates();
    renderCells();
    renderGridLines();
    renderBorder();
    renderStatistics();
}

void PerlerGenerator::renderTitle() {
    if (params_.title.empty()) return;

    double dpiScale = getDpiScale();
    double titleBarHeight = 80.0 * dpiScale;
    double fontSize = 24.0 * dpiScale;

    drawCenteredText(params_.title, canvasWidth_ / 2.0, titleBarHeight / 2.0, fontSize, "#1F2937");
}

void PerlerGenerator::renderCoordinates() {
    if (!params_.options.showCoordinates) return;

    double dpiScale = getDpiScale();
    double cellSize = getCellSize();
    double axisLabelSize = std::max(30.0 * dpiScale, cellSize);
    double fontSize = std::max(10.0 * dpiScale, 12.0);

    double titleBarHeight = !params_.title.empty() ? 80.0 * dpiScale : 0.0;
    double extraLeftMargin = std::max(20.0 * dpiScale, 40.0) + 15.0 * dpiScale;
    double extraTopMargin = params_.options.showCoordinates ? std::max(15.0 * dpiScale, 20.0) : 20.0 * dpiScale;

    double gridStartX = extraLeftMargin + axisLabelSize;
    double gridStartY = titleBarHeight + extraTopMargin + axisLabelSize;

    // X轴坐标
    for (int i = params_.options.gridInterval; i <= pixelData_.width; i += params_.options.gridInterval) {
        if (i <= pixelData_.width) {
            double textX = gridStartX + (i - 1) * cellSize + cellSize / 2.0;
            double textY = titleBarHeight + extraTopMargin + axisLabelSize / 2.0;
            drawCenteredText(std::to_string(i), textX, textY, fontSize, "#666666");
        }
    }

    // Y轴坐标
    for (int j = params_.options.gridInterval; j <= pixelData_.height; j += params_.options.gridInterval) {
        if (j <= pixelData_.height) {
            double textX = extraLeftMargin + axisLabelSize - 5.0 * dpiScale;
            double textY = gridStartY + (j - 1) * cellSize + cellSize / 2.0;

            cairo_set_font_size(cr_, fontSize);
            cairo_text_extents_t extents;
            std::string text = std::to_string(j);
            cairo_text_extents(cr_, text.c_str(), &extents);

            setColor("#666666");
            cairo_move_to(cr_, textX - extents.width, textY + extents.height / 2.0);
            cairo_show_text(cr_, text.c_str());
        }
    }
}

void PerlerGenerator::renderCells() {
    double dpiScale = getDpiScale();
    double cellSize = getCellSize();
    double axisLabelSize = params_.options.showCoordinates ? std::max(30.0 * dpiScale, cellSize) : 0.0;

    double titleBarHeight = !params_.title.empty() ? 80.0 * dpiScale : 0.0;
    double extraLeftMargin = (params_.options.showCoordinates ? std::max(20.0 * dpiScale, 40.0) : 0.0) + 15.0 * dpiScale;
    double extraTopMargin = params_.options.showCoordinates ? std::max(15.0 * dpiScale, 20.0) : 20.0 * dpiScale;

    double gridStartX = extraLeftMargin + axisLabelSize;
    double gridStartY = titleBarHeight + extraTopMargin + axisLabelSize;

    // 动态计算字体大小，确保文字不超出格子边界
    double fontSize = std::max(6.0, std::min(cellSize * 0.4, std::min(12.0 * dpiScale, cellSize / 2.5)));

    for (int j = 0; j < pixelData_.height; j++) {
        for (int i = 0; i < pixelData_.width; i++) {
            const MappedPixel& pixel = pixelData_.mappedData[j][i];

            double drawX = gridStartX + i * cellSize;
            double drawY = gridStartY + j * cellSize;

            if (!pixel.isExternal) {
                // 绘制单元格背景色
                setColor(pixel.color);
                cairo_rectangle(cr_, drawX, drawY, cellSize, cellSize);
                cairo_fill(cr_);

                // 绘制色号文字
                bool isTransparent = (pixel.key == "T01" || pixel.key == "ERASE");
                bool shouldShowLabel = !isTransparent || (isTransparent && params_.options.showTransparentLabels);

                if (shouldShowLabel) {
                    std::string contrastColor = ColorUtils::getContrastColor(pixel.color);
                    drawCenteredText(pixel.key, drawX + cellSize / 2.0, drawY + cellSize / 2.0, fontSize, contrastColor);
                }
            } else {
                // 外部区域，绘制白色背景
                setColor("#FFFFFF");
                cairo_rectangle(cr_, drawX, drawY, cellSize, cellSize);
                cairo_fill(cr_);
            }

            // 绘制单元格边框
            setColor("#DDDDDD");
            cairo_set_line_width(cr_, 0.5 * dpiScale);
            cairo_rectangle(cr_, drawX + 0.5, drawY + 0.5, cellSize, cellSize);
            cairo_stroke(cr_);
        }
    }
}

void PerlerGenerator::renderGridLines() {
    if (!params_.options.showGrid) return;

    double dpiScale = getDpiScale();
    double cellSize = getCellSize();
    double axisLabelSize = params_.options.showCoordinates ? std::max(30.0 * dpiScale, cellSize) : 0.0;

    double titleBarHeight = !params_.title.empty() ? 80.0 * dpiScale : 0.0;
    double extraLeftMargin = (params_.options.showCoordinates ? std::max(20.0 * dpiScale, 40.0) : 0.0) + 15.0 * dpiScale;
    double extraTopMargin = params_.options.showCoordinates ? std::max(15.0 * dpiScale, 20.0) : 20.0 * dpiScale;

    double gridStartX = extraLeftMargin + axisLabelSize;
    double gridStartY = titleBarHeight + extraTopMargin + axisLabelSize;
    double gridWidth = pixelData_.width * cellSize;
    double gridHeight = pixelData_.height * cellSize;

    setColor(params_.options.gridLineColor);
    cairo_set_line_width(cr_, 1.5 * dpiScale);

    // 垂直网格线
    for (int i = params_.options.gridInterval; i < pixelData_.width; i += params_.options.gridInterval) {
        double lineX = gridStartX + i * cellSize;
        cairo_move_to(cr_, lineX, gridStartY);
        cairo_line_to(cr_, lineX, gridStartY + gridHeight);
        cairo_stroke(cr_);
    }

    // 水平网格线
    for (int j = params_.options.gridInterval; j < pixelData_.height; j += params_.options.gridInterval) {
        double lineY = gridStartY + j * cellSize;
        cairo_move_to(cr_, gridStartX, lineY);
        cairo_line_to(cr_, gridStartX + gridWidth, lineY);
        cairo_stroke(cr_);
    }
}

void PerlerGenerator::renderBorder() {
    if (params_.options.outerBorderColor.empty()) return;

    double dpiScale = getDpiScale();
    double cellSize = getCellSize();
    double axisLabelSize = params_.options.showCoordinates ? std::max(30.0 * dpiScale, cellSize) : 0.0;

    double titleBarHeight = !params_.title.empty() ? 80.0 * dpiScale : 0.0;
    double extraLeftMargin = (params_.options.showCoordinates ? std::max(20.0 * dpiScale, 40.0) : 0.0) + 15.0 * dpiScale;
    double extraTopMargin = params_.options.showCoordinates ? std::max(15.0 * dpiScale, 20.0) : 20.0 * dpiScale;

    double borderX = extraLeftMargin + axisLabelSize;
    double borderY = titleBarHeight + extraTopMargin + axisLabelSize;
    double borderWidth = pixelData_.width * cellSize;
    double borderHeight = pixelData_.height * cellSize;

    setColor(params_.options.outerBorderColor);
    cairo_set_line_width(cr_, 2.0 * dpiScale);
    cairo_rectangle(cr_, borderX, borderY, borderWidth, borderHeight);
    cairo_stroke(cr_);
}

void PerlerGenerator::renderStatistics() {
    if (!params_.options.includeStats || colorCounts_.empty()) return;

    double dpiScale = getDpiScale();
    double cellSize = getCellSize();
    double axisLabelSize = params_.options.showCoordinates ? std::max(30.0 * dpiScale, cellSize) : 0.0;

    double titleBarHeight = !params_.title.empty() ? 80.0 * dpiScale : 0.0;
    double extraLeftMargin = (params_.options.showCoordinates ? std::max(20.0 * dpiScale, 40.0) : 0.0) + 15.0 * dpiScale;
    double extraTopMargin = params_.options.showCoordinates ? std::max(15.0 * dpiScale, 20.0) : 20.0 * dpiScale;

    double gridHeight = pixelData_.height * cellSize;
    double gridStartX = extraLeftMargin + axisLabelSize;
    double statsStartY = titleBarHeight + extraTopMargin + gridHeight + axisLabelSize + 16.0 * dpiScale;

    double statsFontSize = 13.0 * dpiScale;
    double swatchSize = std::max(24.0 * dpiScale, 28.0 * dpiScale);

    // 标题
    drawText("豆子用量统计", gridStartX, statsStartY, statsFontSize + 2.0 * dpiScale, "#333333");

    // 准备排序的颜色列表
    std::vector<std::pair<std::string, ColorCount>> sortedColors;
    for (const auto& [key, colorData] : colorCounts_) {
        sortedColors.emplace_back(key, colorData);
    }

    std::sort(sortedColors.begin(), sortedColors.end(),
        [](const auto& a, const auto& b) {
            return compareColorKeys(a.first, b.first);
        });

    // 计算布局
    double gridWidth = pixelData_.width * cellSize;
    double itemWidth = swatchSize + 12.0 * dpiScale + 80.0 * dpiScale; // 估算项目宽度
    int numColumns = std::max(1, static_cast<int>(gridWidth / itemWidth));

    double itemStartY = statsStartY + 2.0 * (statsFontSize + 2.0 * dpiScale);
    double rowHeight = swatchSize + 16.0 * dpiScale;
    double columnWidth = gridWidth / numColumns;

    // 绘制颜色统计项
    for (size_t i = 0; i < sortedColors.size(); i++) {
        const auto& [key, colorData] = sortedColors[i];

        int column = static_cast<int>(i % numColumns);
        int row = static_cast<int>(i / numColumns);

        double itemX = gridStartX + column * columnWidth;
        double itemY = itemStartY + row * rowHeight;

        // 绘制色块
        setColor(colorData.color);
        cairo_rectangle(cr_, itemX, itemY, swatchSize, swatchSize);
        cairo_fill(cr_);

        // 绘制色块边框
        setColor("#DDDDDD");
        cairo_set_line_width(cr_, 1.0);
        cairo_rectangle(cr_, itemX, itemY, swatchSize, swatchSize);
        cairo_stroke(cr_);

        // 在色块内绘制色号
        double swatchFontSize = std::max(10.0 * dpiScale, std::min(16.0 * dpiScale, swatchSize / 3.0));
        std::string contrastColor = ColorUtils::getContrastColor(colorData.color);
        drawCenteredText(key, itemX + swatchSize / 2.0, itemY + swatchSize / 2.0, swatchFontSize, contrastColor);

        // 在右侧绘制数量 - 使用居中绘制
        std::string countText = std::to_string(colorData.count);
        double countX = itemX + swatchSize + 12.0 * dpiScale + 40.0 * dpiScale; // 数量文字的中心位置
        drawCenteredText(countText, countX, itemY + swatchSize / 2.0, statsFontSize, "#333333");
    }

    // 绘制总计
    int numRows = static_cast<int>(std::ceil(static_cast<double>(sortedColors.size()) / numColumns));
    double totalY = itemStartY + numRows * rowHeight + 10.0 * dpiScale;
    std::string totalText = "总计: " + std::to_string(totalBeadCount_) + " 颗";
    drawText(totalText, gridStartX, totalY, statsFontSize + 1.0 * dpiScale, "#333333");
}

void PerlerGenerator::setColor(const std::string& hexColor) {
    try {
        RgbColor rgb = ColorUtils::hexToRgb(hexColor);
        cairo_set_source_rgb(cr_, rgb.r / 255.0, rgb.g / 255.0, rgb.b / 255.0);
    } catch (const std::exception& e) {
        // 默认使用黑色
        cairo_set_source_rgb(cr_, 0.0, 0.0, 0.0);
    }
}

void PerlerGenerator::drawText(const std::string& text, double x, double y, double fontSize, const std::string& color) {
    setColor(color);
    cairo_select_font_face(cr_, "DejaVu Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr_, fontSize);
    cairo_move_to(cr_, x, y);
    cairo_show_text(cr_, text.c_str());
}

void PerlerGenerator::drawCenteredText(const std::string& text, double x, double y, double fontSize, const std::string& color) {
    setColor(color);

    // 尝试多种中文字体，确保中文正确显示
    cairo_select_font_face(cr_, "DejaVu Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr_, fontSize);

    cairo_text_extents_t extents;
    cairo_text_extents(cr_, text.c_str(), &extents);

    // 更精确的居中计算 - 使用标准的居中方法
    double textX = x - extents.width / 2.0;
    double textY = y + extents.height / 2.0;

    cairo_move_to(cr_, textX, textY);
    cairo_show_text(cr_, text.c_str());
}

double PerlerGenerator::getTextWidth(const std::string& text, double fontSize) {
    cairo_set_font_size(cr_, fontSize);
    cairo_text_extents_t extents;
    cairo_text_extents(cr_, text.c_str(), &extents);
    return extents.width;
}

double PerlerGenerator::getDpiScale() const {
    return params_.options.dpi / 150.0;
}

double PerlerGenerator::getCellSize() const {
    double dpiScale = getDpiScale();

    if (params_.renderMode == RenderMode::FIXED_WIDTH && params_.options.fixedWidth > 0) {
        // 固定宽度模式
        double axisSpace = params_.options.showCoordinates ? std::max(30.0, 20.0) : 0.0;
        double margins = 70.0; // 左右边距估算
        double availableWidth = params_.options.fixedWidth - axisSpace - margins;
        return std::max(10.0, std::floor(availableWidth / pixelData_.width));
    } else {
        // DPI模式
        double baseCellSize = 30.0;
        return std::round(baseCellSize * dpiScale);
    }
}
