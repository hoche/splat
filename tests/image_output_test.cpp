#include <fstream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>

// Include the ImageWriter header
#include "../src/imagewriter.h"

// Test fixture for ImageWriter class
class ImageWriterTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Initialize common test data
        width = 100;
        height = 100;
        north = 45.0;
        south = 44.0;
        east = -122.0;
        west = -123.0;
    }

    void TearDown() override {
        // Clean up any test files
        std::remove("test_output.ppm");
        std::remove("test_output.png");
        std::remove("test_output.jpg");
    }

    int width;
    int height;
    double north;
    double south;
    double east;
    double west;
};

// Test ImageWriter construction for PPM output
TEST_F(ImageWriterTest, ConstructPPMWriter) {
    EXPECT_NO_THROW({
        ImageWriter writer("test_output.ppm", IMAGETYPE_PPM, width, height,
                           north, south, east, west);
    }) << "ImageWriter construction should succeed for PPM format";
}

// Test ImageWriter construction with invalid filename
TEST_F(ImageWriterTest, ConstructWithInvalidFilename) {
    EXPECT_THROW(
        {
            ImageWriter writer("/invalid/path/test.ppm", IMAGETYPE_PPM, width,
                               height, north, south, east, west);
        },
        std::invalid_argument)
        << "ImageWriter should throw on invalid filename";
}

// Test writing a simple PPM image
TEST_F(ImageWriterTest, WritePPMImage) {
    ImageWriter writer("test_output.ppm", IMAGETYPE_PPM, width, height, north,
                       south, east, west);

    // Write some pixel data
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // Create a simple gradient pattern (RGB pixel)
            Pixel pixel = (255 << 24) | ((x * 255 / width) << 16) |
                          ((y * 255 / height) << 8) | 128;
            writer.AppendPixel(pixel);
        }
        writer.EmitLine();
    }

    writer.Finish();

    // Verify file was created
    std::ifstream file("test_output.ppm");
    EXPECT_TRUE(file.good()) << "Output file should exist";
    file.close();
}

// Test writing pixels with AppendPixel and EmitLine
TEST_F(ImageWriterTest, AppendPixelAndEmitLine) {
    ImageWriter writer("test_output.ppm", IMAGETYPE_PPM, 10, 10, north, south,
                       east, west);

    // Write one line of pixels
    for (int x = 0; x < 10; x++) {
        Pixel pixel = 0xFF000000;  // Black pixel
        writer.AppendPixel(pixel);
    }
    writer.EmitLine();

    writer.Finish();

    // Verify file exists
    std::ifstream file("test_output.ppm");
    EXPECT_TRUE(file.good());
    file.close();
}

// Test that initialized flag is set correctly
TEST_F(ImageWriterTest, InitializationFlag) {
    ImageWriter writer("test_output.ppm", IMAGETYPE_PPM, width, height, north,
                       south, east, west);
    EXPECT_TRUE(writer.m_initialized)
        << "ImageWriter should be initialized after construction";
}

// Parameterized test for different image sizes
class ImageWriterSizeTest
    : public ImageWriterTest,
      public ::testing::WithParamInterface<std::tuple<int, int>> {
  protected:
    void SetUp() override {
        ImageWriterTest::SetUp();
        width = std::get<0>(GetParam());
        height = std::get<1>(GetParam());
    }
};

// Test ImageWriter with various image sizes
TEST_P(ImageWriterSizeTest, VariousImageSizes) {
    EXPECT_NO_THROW({
        ImageWriter writer("test_output.ppm", IMAGETYPE_PPM, width, height,
                           north, south, east, west);

        // Write a simple image
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                Pixel pixel = 0xFFFFFFFF;  // White pixel
                writer.AppendPixel(pixel);
            }
            writer.EmitLine();
        }
        writer.Finish();
    }) << "ImageWriter should handle size "
       << width << "x" << height;

    std::remove("test_output.ppm");
}

// Instantiate parameterized test with different image sizes
INSTANTIATE_TEST_SUITE_P(
    ImageSizes, ImageWriterSizeTest,
    ::testing::Values(std::make_tuple(1, 1),      // Minimal size
                      std::make_tuple(10, 10),    // Small size
                      std::make_tuple(100, 100),  // Standard size
                      std::make_tuple(256, 256)   // Larger size
                      ));
