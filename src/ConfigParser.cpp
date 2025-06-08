#include "ConfigParser.h"
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <regex>
#include <cairo.h>

namespace fs = std::filesystem;

ConfigParser::ConfigParser() {
    setDefaults();
}

ConfigParser::~ConfigParser() = default;

void ConfigParser::setDefaults() {
    inputImage_ = "";
    outputDirectory_ = "./perler_output";
    paletteFile_ = "144-perler-palette.json";  // 默认使用144色调色板
    colorSystem_ = "MARD";
    title_ = "";

    granularity_ = -1;  // -1表示使用原图宽度作为默认granularity
    similarityThreshold_ = 30;
    pixelationMode_ = PixelationMode::DOMINANT;

    renderMode_ = RenderMode::DPI_BASED;
    dpi_ = 300;
    fixedWidth_ = 800;

    showGrid_ = true;
    showCoordinates_ = true;
    includeStats_ = true;
    showTransparentLabels_ = false;
    gridLineColor_ = "#CCCCCC";
    outerBorderColor_ = "#000000";
    gridInterval_ = 10;

    verbose_ = false;
    showHelp_ = false;
    showVersion_ = false;
}

bool ConfigParser::parseArguments(int argc, char* argv[]) {
    if (argc < 2) {
        showHelp_ = true;
        return true;
    }

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        std::string nextArg = (i + 1 < argc) ? argv[i + 1] : "";

        if (!parseArgument(arg, nextArg, i)) {
            return false;
        }
    }

    return true;
}

bool ConfigParser::parseArgument(const std::string& arg, const std::string& nextArg, int& index) {
    if (arg == "-h" || arg == "--help") {
        showHelp_ = true;
        return true;
    }

    if (arg == "-v" || arg == "--version") {
        showVersion_ = true;
        return true;
    }

    if (arg == "-i" || arg == "--input") {
        if (nextArg.empty() || nextArg[0] == '-') {
            std::cerr << "Error: Missing input image path after " << arg << std::endl;
            return false;
        }
        inputImage_ = nextArg;
        ++index;
        return true;
    }

    if (arg == "-o" || arg == "--output") {
        if (nextArg.empty() || nextArg[0] == '-') {
            std::cerr << "Error: Missing output directory after " << arg << std::endl;
            return false;
        }
        outputDirectory_ = nextArg;
        ++index;
        return true;
    }

    if (arg == "-p" || arg == "--palette") {
        if (nextArg.empty() || nextArg[0] == '-') {
            std::cerr << "Error: Missing palette file path after " << arg << std::endl;
            return false;
        }
        paletteFile_ = nextArg;
        ++index;
        return true;
    }

    if (arg == "-c" || arg == "--color-system") {
        if (nextArg.empty() || nextArg[0] == '-') {
            std::cerr << "Error: Missing color system after " << arg << std::endl;
            return false;
        }
        colorSystem_ = nextArg;
        ++index;
        return true;
    }

    if (arg == "-t" || arg == "--title") {
        if (nextArg.empty() || nextArg[0] == '-') {
            std::cerr << "Error: Missing title after " << arg << std::endl;
            return false;
        }
        title_ = nextArg;
        ++index;
        return true;
    }

    if (arg == "-g" || arg == "--granularity") {
        if (nextArg.empty() || nextArg[0] == '-') {
            std::cerr << "Error: Missing granularity value after " << arg << std::endl;
            return false;
        }
        try {
            granularity_ = std::stoi(nextArg);
        } catch (const std::exception& e) {
            std::cerr << "Error: Invalid granularity value: " << nextArg << std::endl;
            return false;
        }
        ++index;
        return true;
    }

    if (arg == "-s" || arg == "--similarity") {
        if (nextArg.empty() || nextArg[0] == '-') {
            std::cerr << "Error: Missing similarity threshold after " << arg << std::endl;
            return false;
        }
        try {
            similarityThreshold_ = std::stoi(nextArg);
        } catch (const std::exception& e) {
            std::cerr << "Error: Invalid similarity threshold: " << nextArg << std::endl;
            return false;
        }
        ++index;
        return true;
    }

    if (arg == "-m" || arg == "--mode") {
        if (nextArg.empty() || nextArg[0] == '-') {
            std::cerr << "Error: Missing pixelation mode after " << arg << std::endl;
            return false;
        }
        if (nextArg == "dominant" || nextArg == "cartoon") {
            pixelationMode_ = PixelationMode::DOMINANT;
        } else if (nextArg == "average" || nextArg == "realistic") {
            pixelationMode_ = PixelationMode::AVERAGE;
        } else {
            std::cerr << "Error: Invalid pixelation mode: " << nextArg << std::endl;
            std::cerr << "Valid modes: dominant, cartoon, average, realistic" << std::endl;
            return false;
        }
        ++index;
        return true;
    }

    if (arg == "--render-mode") {
        if (nextArg.empty() || nextArg[0] == '-') {
            std::cerr << "Error: Missing render mode after " << arg << std::endl;
            return false;
        }
        if (nextArg == "dpi") {
            renderMode_ = RenderMode::DPI_BASED;
        } else if (nextArg == "fixed") {
            renderMode_ = RenderMode::FIXED_WIDTH;
        } else {
            std::cerr << "Error: Invalid render mode: " << nextArg << std::endl;
            std::cerr << "Valid modes: dpi, fixed" << std::endl;
            return false;
        }
        ++index;
        return true;
    }

    if (arg == "--dpi") {
        if (nextArg.empty() || nextArg[0] == '-') {
            std::cerr << "Error: Missing DPI value after " << arg << std::endl;
            return false;
        }
        try {
            dpi_ = std::stoi(nextArg);
        } catch (const std::exception& e) {
            std::cerr << "Error: Invalid DPI value: " << nextArg << std::endl;
            return false;
        }
        ++index;
        return true;
    }

    if (arg == "--fixed-width") {
        if (nextArg.empty() || nextArg[0] == '-') {
            std::cerr << "Error: Missing fixed width value after " << arg << std::endl;
            return false;
        }
        try {
            fixedWidth_ = std::stoi(nextArg);
        } catch (const std::exception& e) {
            std::cerr << "Error: Invalid fixed width value: " << nextArg << std::endl;
            return false;
        }
        ++index;
        return true;
    }

    if (arg == "--grid-interval") {
        if (nextArg.empty() || nextArg[0] == '-') {
            std::cerr << "Error: Missing grid interval value after " << arg << std::endl;
            return false;
        }
        try {
            gridInterval_ = std::stoi(nextArg);
        } catch (const std::exception& e) {
            std::cerr << "Error: Invalid grid interval value: " << nextArg << std::endl;
            return false;
        }
        ++index;
        return true;
    }

    if (arg == "--grid-color") {
        if (nextArg.empty() || nextArg[0] == '-') {
            std::cerr << "Error: Missing grid line color after " << arg << std::endl;
            return false;
        }
        gridLineColor_ = nextArg;
        ++index;
        return true;
    }

    if (arg == "--border-color") {
        if (nextArg.empty() || nextArg[0] == '-') {
            std::cerr << "Error: Missing border color after " << arg << std::endl;
            return false;
        }
        outerBorderColor_ = nextArg;
        ++index;
        return true;
    }

    // Boolean flags
    if (arg == "--no-grid") {
        showGrid_ = false;
        return true;
    }

    if (arg == "--no-coordinates") {
        showCoordinates_ = false;
        return true;
    }

    if (arg == "--no-stats") {
        includeStats_ = false;
        return true;
    }

    if (arg == "--show-transparent-labels") {
        showTransparentLabels_ = true;
        return true;
    }

    if (arg == "--verbose") {
        verbose_ = true;
        return true;
    }

    // 如果没有'-'前缀，可能是输入文件
    if (arg[0] != '-' && inputImage_.empty()) {
        inputImage_ = arg;
        return true;
    }

    std::cerr << "Error: Unknown argument: " << arg << std::endl;
    return false;
}

void ConfigParser::showHelp(const std::string& programName) const {
    std::cout << "Perler Beads Pattern Generator\n";
    std::cout << "Usage: " << programName << " [OPTIONS] <input_image>\n\n";

    std::cout << "REQUIRED:\n";
    std::cout << "  input_image                Input image file (jpg, png, gif, bmp, webp)\n\n";

    std::cout << "PROCESSING OPTIONS:\n";
    std::cout << "  -g, --granularity <N>      Number of pixels along width (10-200, default: 50)\n";
    std::cout << "  -s, --similarity <N>       Color similarity threshold (0-100, default: 30)\n";
    std::cout << "  -m, --mode <MODE>          Pixelation mode:\n";
    std::cout << "                               dominant|cartoon - Use dominant color (default)\n";
    std::cout << "                               average|realistic - Use average color\n\n";

    std::cout << "OUTPUT OPTIONS:\n";
    std::cout << "  -o, --output <DIR>         Output directory (default: ./perler_output)\n";
    std::cout << "  -t, --title <TITLE>        Title to display on pattern\n";
    std::cout << "      --render-mode <MODE>   Render mode: dpi|fixed (default: dpi)\n";
    std::cout << "      --dpi <N>              DPI for rendering (default: 300)\n";
    std::cout << "      --fixed-width <N>      Fixed width in pixels (default: 800)\n\n";

    std::cout << "PALETTE OPTIONS:\n";
    std::cout << "  -p, --palette <FILE>       Custom palette file (JSON format)\n";
    std::cout << "  -c, --color-system <SYS>   Color system (default: MARD)\n\n";

    std::cout << "GRID & DISPLAY OPTIONS:\n";
    std::cout << "      --no-grid              Disable grid lines\n";
    std::cout << "      --no-coordinates       Disable coordinate numbers\n";
    std::cout << "      --no-stats             Disable color statistics\n";
    std::cout << "      --grid-interval <N>    Grid line interval (default: 10)\n";
    std::cout << "      --grid-color <COLOR>   Grid line color (default: #CCCCCC)\n";
    std::cout << "      --border-color <COLOR> Outer border color (default: #000000)\n";
    std::cout << "      --show-transparent-labels  Show transparent color labels\n\n";

    std::cout << "OTHER OPTIONS:\n";
    std::cout << "      --verbose              Enable verbose output\n";
    std::cout << "  -h, --help                 Show this help message\n";
    std::cout << "  -v, --version              Show version information\n\n";

    std::cout << "EXAMPLES:\n";
    std::cout << "  " << programName << " image.jpg\n";
    std::cout << "  " << programName << " -g 100 -s 25 -t \"My Pattern\" image.png\n";
    std::cout << "  " << programName << " --mode average --dpi 150 --no-grid image.jpg\n";
    std::cout << "  " << programName << " -p custom_palette.json -o ./output image.jpg\n";
}

void ConfigParser::showVersion() const {
    std::cout << "Perler Beads Pattern Generator v1.0.0\n";
    std::cout << "Built with Cairo " << cairo_version_string() << "\n";
    std::cout << "Compatible with MARD color system\n";
}

ValidationResult ConfigParser::validate() const {
    ValidationResult result;
    result.isValid = true;

    // 检查帮助和版本标志
    if (showHelp_ || showVersion_) {
        return result;
    }

    // 检查输入图片
    if (inputImage_.empty()) {
        result.isValid = false;
        result.errorMessage = "Input image is required";
        return result;
    }

    if (!fileExists(inputImage_)) {
        result.isValid = false;
        result.errorMessage = "Input image file does not exist: " + inputImage_;
        return result;
    }

    // 检查输出目录
    if (!ensureDirectoryExists(outputDirectory_)) {
        result.isValid = false;
        result.errorMessage = "Cannot create output directory: " + outputDirectory_;
        return result;
    }

    // 检查调色板文件
    if (!paletteFile_.empty() && !fileExists(paletteFile_)) {
        result.isValid = false;
        result.errorMessage = "Palette file does not exist: " + paletteFile_;
        return result;
    }

    // 检查数值范围
    if (granularity_ < 10 || granularity_ > 200) {
        result.isValid = false;
        result.errorMessage = "Granularity must be between 10 and 200";
        return result;
    }

    if (similarityThreshold_ < 0 || similarityThreshold_ > 100) {
        result.isValid = false;
        result.errorMessage = "Similarity threshold must be between 0 and 100";
        return result;
    }

    if (dpi_ < 50 || dpi_ > 1200) {
        result.isValid = false;
        result.errorMessage = "DPI must be between 50 and 1200";
        return result;
    }

    if (fixedWidth_ < 100 || fixedWidth_ > 4000) {
        result.isValid = false;
        result.errorMessage = "Fixed width must be between 100 and 4000 pixels";
        return result;
    }

    if (gridInterval_ < 1 || gridInterval_ > 50) {
        result.isValid = false;
        result.errorMessage = "Grid interval must be between 1 and 50";
        return result;
    }

    // 检查颜色格式
    if (!isValidColor(gridLineColor_)) {
        result.isValid = false;
        result.errorMessage = "Invalid grid line color format: " + gridLineColor_;
        return result;
    }

    if (!isValidColor(outerBorderColor_)) {
        result.isValid = false;
        result.errorMessage = "Invalid border color format: " + outerBorderColor_;
        return result;
    }

    return result;
}

bool ConfigParser::isValidColor(const std::string& color) const {
    if (color.empty()) return false;

    // 检查十六进制颜色格式 #RRGGBB
    std::regex hexPattern("^#[0-9A-Fa-f]{6}$");
    return std::regex_match(color, hexPattern);
}

bool ConfigParser::fileExists(const std::string& path) const {
    return fs::exists(path) && fs::is_regular_file(path);
}

bool ConfigParser::ensureDirectoryExists(const std::string& path) const {
    try {
        if (!fs::exists(path)) {
            return fs::create_directories(path);
        }
        return fs::is_directory(path);
    } catch (const std::exception& e) {
        std::cerr << "Error creating directory: " << e.what() << std::endl;
        return false;
    }
}
