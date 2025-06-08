#include "ImageProcessor.h"
#include "ColorUtils.h"
#include "lodepng.h"
#include <iostream>
#include <map>
#include <algorithm>
#include <cmath>

ImageProcessor::ImageProcessor()
    : imageData_(nullptr), imageWidth_(0), imageHeight_(0), pixelationMode_(PixelationMode::DOMINANT) {
}

ImageProcessor::~ImageProcessor() {
    freeImageData();
}

bool ImageProcessor::loadImage(const std::string& filename) {
    freeImageData();

    std::vector<unsigned char> image;
    unsigned width, height;

    // 使用lodepng加载PNG图像
    unsigned error = lodepng::decode(image, width, height, filename);

    if (error) {
        std::cerr << "Failed to load image: " << lodepng_error_text(error) << std::endl;
        return false;
    }

    imageWidth_ = static_cast<int>(width);
    imageHeight_ = static_cast<int>(height);

    // 分配并复制图像数据
    size_t dataSize = imageWidth_ * imageHeight_ * 4; // RGBA
    imageData_ = new unsigned char[dataSize];
    std::copy(image.begin(), image.end(), imageData_);

    std::cout << "Loaded image: " << filename
              << " (" << imageWidth_ << "x" << imageHeight_ << ")" << std::endl;

    return true;
}

void ImageProcessor::setPalette(const ColorPalette& palette) {
    palette_ = palette;
}

PixelData ImageProcessor::calculatePixelGrid(int gridWidth, int gridHeight) {
    if (!isImageLoaded()) {
        throw std::runtime_error("No image loaded");
    }

    if (!palette_.isValid()) {
        throw std::runtime_error("Invalid palette");
    }

    PixelData result;
    result.width = gridWidth;
    result.height = gridHeight;
    result.mappedData.resize(gridHeight, std::vector<MappedPixel>(gridWidth));

    // 计算每个网格单元的尺寸
    double cellWidth = static_cast<double>(imageWidth_) / gridWidth;
    double cellHeight = static_cast<double>(imageHeight_) / gridHeight;

    PaletteColor fallbackColor = palette_.getTransparentFallbackColor();

    for (int j = 0; j < gridHeight; j++) {
        for (int i = 0; i < gridWidth; i++) {
            // 计算单元格在原图中的位置
            int startX = static_cast<int>(i * cellWidth);
            int startY = static_cast<int>(j * cellHeight);
            int endX = static_cast<int>((i + 1) * cellWidth);
            int endY = static_cast<int>((j + 1) * cellHeight);

            // 确保不超出图像边界
            endX = std::min(endX, imageWidth_);
            endY = std::min(endY, imageHeight_);

            int actualCellWidth = endX - startX;
            int actualCellHeight = endY - startY;

            if (actualCellWidth <= 0 || actualCellHeight <= 0) {
                // 外部区域，使用备用色
                result.mappedData[j][i] = MappedPixel(fallbackColor.key, fallbackColor.hex, true);
                continue;
            }

            // 计算单元格的代表色
            RgbColor representativeColor = calculateCellRepresentativeColor(
                startX, startY, actualCellWidth, actualCellHeight);

            // 查找最接近的调色板颜色
            PaletteColor closestColor = palette_.findClosestColor(representativeColor);
            result.mappedData[j][i] = MappedPixel(closestColor.key, closestColor.hex, false);
        }
    }

    std::cout << "Generated pixel grid: " << gridWidth << "x" << gridHeight << std::endl;
    return result;
}

void ImageProcessor::getImageSize(int& width, int& height) const {
    width = imageWidth_;
    height = imageHeight_;
}

ColorCountMap ImageProcessor::calculateColorCounts(const PixelData& pixelData) {
    ColorCountMap colorCounts;

    for (const auto& row : pixelData.mappedData) {
        for (const auto& pixel : row) {
            auto it = colorCounts.find(pixel.key);
            if (it != colorCounts.end()) {
                it->second.count++;
            } else {
                colorCounts[pixel.key] = ColorCount(1, pixel.color);
            }
        }
    }

    return colorCounts;
}

ColorCountMap ImageProcessor::filterColorCountsForBeadUsage(const ColorCountMap& colorCounts, bool excludeTransparent) {
    if (!excludeTransparent) {
        return colorCounts;
    }

    ColorCountMap filteredCounts;

    for (const auto& [key, colorData] : colorCounts) {
        // 排除白色透明色 (#FFFFFF)
        if (colorData.color != "#FFFFFF") {
            filteredCounts[key] = colorData;
        }
    }

    return filteredCounts;
}

RgbColor ImageProcessor::calculateCellRepresentativeColor(int startX, int startY, int cellWidth, int cellHeight) {
    switch (pixelationMode_) {
        case PixelationMode::DOMINANT:
            return calculateDominantColor(startX, startY, cellWidth, cellHeight);
        case PixelationMode::AVERAGE:
            return calculateAverageColor(startX, startY, cellWidth, cellHeight);
        default:
            return calculateDominantColor(startX, startY, cellWidth, cellHeight);
    }
}

RgbColor ImageProcessor::calculateDominantColor(int startX, int startY, int cellWidth, int cellHeight) {
    std::map<RgbColor, int> colorCounts;
    int totalPixels = 0;

    for (int y = startY; y < startY + cellHeight; y++) {
        for (int x = startX; x < startX + cellWidth; x++) {
            if (x >= imageWidth_ || y >= imageHeight_) continue;

            // 检查alpha通道，忽略完全透明的像素
            int index = (y * imageWidth_ + x) * 4;
            if (imageData_[index + 3] < 128) continue; // alpha < 50%

            RgbColor pixelColor = getPixelColor(x, y);
            colorCounts[pixelColor]++;
            totalPixels++;
        }
    }

    if (totalPixels == 0) {
        // 如果没有有效像素，返回白色
        return RgbColor(255, 255, 255);
    }

    // 找到出现次数最多的颜色
    RgbColor dominantColor(255, 255, 255);
    int maxCount = 0;

    for (const auto& [color, count] : colorCounts) {
        if (count > maxCount) {
            maxCount = count;
            dominantColor = color;
        }
    }

    return dominantColor;
}

RgbColor ImageProcessor::calculateAverageColor(int startX, int startY, int cellWidth, int cellHeight) {
    long long rSum = 0, gSum = 0, bSum = 0;
    int validPixels = 0;

    for (int y = startY; y < startY + cellHeight; y++) {
        for (int x = startX; x < startX + cellWidth; x++) {
            if (x >= imageWidth_ || y >= imageHeight_) continue;

            // 检查alpha通道，忽略完全透明的像素
            int index = (y * imageWidth_ + x) * 4;
            if (imageData_[index + 3] < 128) continue; // alpha < 50%

            RgbColor pixelColor = getPixelColor(x, y);
            rSum += pixelColor.r;
            gSum += pixelColor.g;
            bSum += pixelColor.b;
            validPixels++;
        }
    }

    if (validPixels == 0) {
        // 如果没有有效像素，返回白色
        return RgbColor(255, 255, 255);
    }

    return RgbColor(
        static_cast<int>(rSum / validPixels),
        static_cast<int>(gSum / validPixels),
        static_cast<int>(bSum / validPixels)
    );
}

RgbColor ImageProcessor::getPixelColor(int x, int y) const {
    if (x < 0 || x >= imageWidth_ || y < 0 || y >= imageHeight_ || !imageData_) {
        return RgbColor(0, 0, 0);
    }

    int index = (y * imageWidth_ + x) * 4; // RGBA format
    return RgbColor(
        imageData_[index],     // R
        imageData_[index + 1], // G
        imageData_[index + 2]  // B
    );
}

void ImageProcessor::freeImageData() {
    if (imageData_) {
        delete[] imageData_;
        imageData_ = nullptr;
    }
    imageWidth_ = 0;
    imageHeight_ = 0;
}
