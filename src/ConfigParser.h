#pragma once

#include <string>
#include <vector>
#include <map>
#include "Types.h"

/**
 * @brief 命令行参数解析器
 *
 * 负责解析和验证命令行参数，提供对配置选项的访问。
 */
class ConfigParser {
public:
    ConfigParser();
    ~ConfigParser();

    /**
     * @brief 解析命令行参数
     * @param argc 参数个数
     * @param argv 参数数组
     * @return 解析是否成功
     */
    bool parseArguments(int argc, char* argv[]);

    /**
     * @brief 显示使用帮助信息
     */
    void showHelp(const std::string& programName) const;

    /**
     * @brief 显示版本信息
     */
    void showVersion() const;

    // Getters
    const std::string& getInputImage() const { return inputImage_; }
    const std::string& getOutputDirectory() const { return outputDirectory_; }
    const std::string& getPaletteFile() const { return paletteFile_; }
    const std::string& getColorSystem() const { return colorSystem_; }
    const std::string& getTitle() const { return title_; }

    int getGranularity() const { return granularity_; }
    int getSimilarityThreshold() const { return similarityThreshold_; }
    int getDpi() const { return dpi_; }
    int getFixedWidth() const { return fixedWidth_; }

    PixelationMode getPixelationMode() const { return pixelationMode_; }
    RenderMode getRenderMode() const { return renderMode_; }

    bool getShowGrid() const { return showGrid_; }
    bool getShowCoordinates() const { return showCoordinates_; }
    bool getIncludeStats() const { return includeStats_; }
    bool getShowTransparentLabels() const { return showTransparentLabels_; }
    bool getVerbose() const { return verbose_; }

    // Status checks
    bool shouldShowHelp() const { return showHelp_; }
    bool shouldShowVersion() const { return showVersion_; }

    const std::string& getGridLineColor() const { return gridLineColor_; }
    const std::string& getOuterBorderColor() const { return outerBorderColor_; }

    int getGridInterval() const { return gridInterval_; }

    /**
     * @brief 验证所有参数的有效性
     * @return 验证结果和错误信息
     */
    ValidationResult validate() const;

private:
    // 输入输出
    std::string inputImage_;
    std::string outputDirectory_;
    std::string paletteFile_;
    std::string colorSystem_;
    std::string title_;

    // 处理参数
    int granularity_;
    int similarityThreshold_;
    PixelationMode pixelationMode_;

    // 渲染参数
    RenderMode renderMode_;
    int dpi_;
    int fixedWidth_;

    // 网格和显示选项
    bool showGrid_;
    bool showCoordinates_;
    bool includeStats_;
    bool showTransparentLabels_;
    std::string gridLineColor_;
    std::string outerBorderColor_;
    int gridInterval_;

    // 其他选项
    bool verbose_;
    bool showHelp_;
    bool showVersion_;

    /**
     * @brief 解析单个参数
     * @param arg 参数字符串
     * @param nextArg 下一个参数（可能为空）
     * @param index 当前参数索引
     * @return 是否成功解析
     */
    bool parseArgument(const std::string& arg, const std::string& nextArg, int& index);

    /**
     * @brief 设置默认值
     */
    void setDefaults();

    /**
     * @brief 验证颜色字符串格式
     * @param color 颜色字符串（十六进制格式）
     * @return 是否为有效颜色
     */
    bool isValidColor(const std::string& color) const;

    /**
     * @brief 验证文件路径是否存在
     * @param path 文件路径
     * @return 文件是否存在
     */
    bool fileExists(const std::string& path) const;

    /**
     * @brief 创建目录（如果不存在）
     * @param path 目录路径
     * @return 是否成功创建或已存在
     */
    bool ensureDirectoryExists(const std::string& path) const;
};
