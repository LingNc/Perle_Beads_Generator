#include <iostream>
#include <memory>
#include <stdexcept>
#include <cstdlib>
#include <cmath>
#include <algorithm>

#include "ConfigParser.h"
#include "ColorPalette.h"
#include "ImageProcessor.h"
#include "PerlerGenerator.h"

int main(int argc, char* argv[]) {
    try {
        // Parse command line arguments
        ConfigParser config;
        if (!config.parseArguments(argc, argv)) {
            return EXIT_FAILURE; // Error in parsing
        }

        // Check if help or version was requested
        if (config.shouldShowHelp()) {
            config.showHelp(argv[0]);
            return EXIT_SUCCESS;
        }

        if (config.shouldShowVersion()) {
            config.showVersion();
            return EXIT_SUCCESS;
        }

        // Validate required parameters
        if (config.getInputImage().empty()) {
            std::cerr << "Error: Input image is required. Use -i or --input to specify an image file." << std::endl;
            config.showHelp(argv[0]);
            return EXIT_FAILURE;
        }

        std::cout << "Perler Bead Pattern Generator (C++)" << std::endl;
        std::cout << "Processing: " << config.getInputImage() << std::endl;

        // Initialize color palette
        std::cout << "Loading color palette..." << std::endl;
        auto palette = std::make_unique<ColorPalette>();

        if (!palette->loadDefaultPalette()) {
            std::cerr << "Error: Failed to load default palette" << std::endl;
            return EXIT_FAILURE;
        }

        // Load custom palette if specified
        if (!config.getPaletteFile().empty()) {
            if (!palette->loadPaletteFromFile(config.getPaletteFile())) {
                std::cerr << "Warning: Failed to load custom palette file, using default" << std::endl;
            }
        }

        std::cout << "Loaded " << palette->size() << " colors in palette" << std::endl;

        // Process input image
        std::cout << "Loading and processing image..." << std::endl;
        ImageProcessor processor;

        if (!processor.loadImage(config.getInputImage())) {
            std::cerr << "Error: Failed to load image: " << config.getInputImage() << std::endl;
            return EXIT_FAILURE;
        }

        // Set palette and pixelation mode
        processor.setPalette(*palette);
        processor.setPixelationMode(config.getPixelationMode());

        // Get image dimensions for grid calculation
        int imageWidth, imageHeight;
        processor.getImageSize(imageWidth, imageHeight);

        // Calculate grid dimensions based on granularity
        int gridWidth;
        if (config.getGranularity() <= 0) {
            // Default: use original image width
            gridWidth = imageWidth;
        } else {
            gridWidth = config.getGranularity();
            // If granularity is larger than image width, use image width
            if (gridWidth > imageWidth) {
                gridWidth = imageWidth;
            }
        }

        double aspectRatio = static_cast<double>(imageHeight) / static_cast<double>(imageWidth);
        int gridHeight = std::max(1, static_cast<int>(std::round(gridWidth * aspectRatio)));

        // Calculate pixel grid
        PixelData pixelData = processor.calculatePixelGrid(gridWidth, gridHeight);

        std::cout << "Generated " << gridWidth << "x" << gridHeight
                  << " pixel grid (" << (gridWidth * gridHeight) << " beads)" << std::endl;

        // Generate color statistics
        ColorCountMap colorCounts = processor.calculateColorCounts(pixelData);
        ColorCountMap filteredCounts = processor.filterColorCountsForBeadUsage(colorCounts);

        std::cout << "Using " << filteredCounts.size() << " different colors" << std::endl;

        // Print color usage summary
        std::cout << "\nColor usage:" << std::endl;
        for (const auto& [colorKey, colorCount] : filteredCounts) {
            PaletteColor* paletteColor = palette->findColorByKey(colorKey);
            if (paletteColor) {
                std::cout << "  " << paletteColor->key << " (" << paletteColor->hex << "): "
                          << colorCount.count << " beads" << std::endl;
            }
        }

        // Generate output
        std::cout << "\nGenerating pattern..." << std::endl;
        PerlerGenerator generator;
        generator.setPalette(*palette);

        // Create generate parameters
        GenerateImageParams params;
        params.title = config.getTitle().empty() ? "Perler Bead Pattern" : config.getTitle();
        params.pixelData = pixelData;
        params.renderMode = (config.getFixedWidth() > 0) ? RenderMode::FIXED_WIDTH : RenderMode::DPI_BASED;

        // Set download options
        params.options.showGrid = config.getShowGrid();
        params.options.showCoordinates = config.getShowCoordinates();
        params.options.includeStats = config.getIncludeStats();
        params.options.dpi = config.getDpi();
        params.options.fixedWidth = config.getFixedWidth();

        // Generate output filename if not specified
        std::string outputPath = config.getOutputDirectory();
        if (outputPath.empty()) {
            outputPath = "output.png";
        } else {
            // Ensure output path ends with .png
            if (outputPath.find_last_of('.') == std::string::npos) {
                outputPath += ".png";
            }
        }

        bool success = generator.generateFromImage(config.getInputImage(), params);
        if (success) {
            success = generator.saveImage(outputPath);
        }

        if (success) {
            std::cout << "Pattern saved to: " << outputPath << std::endl;
            std::cout << "Generation completed successfully!" << std::endl;

            // Optionally save color statistics
            std::string statsPath = outputPath;
            size_t dotPos = statsPath.find_last_of('.');
            if (dotPos != std::string::npos) {
                statsPath = statsPath.substr(0, dotPos) + "_stats.txt";
            } else {
                statsPath += "_stats.txt";
            }
            generator.saveColorStats(statsPath, params.title);
            std::cout << "Color statistics saved to: " << statsPath << std::endl;
        } else {
            std::cerr << "Error: Failed to generate pattern" << std::endl;
            return EXIT_FAILURE;
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    } catch (...) {
        std::cerr << "Error: Unknown exception occurred" << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
