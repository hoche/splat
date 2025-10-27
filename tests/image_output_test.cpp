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

#ifdef HAVE_LIBPNG
// Test PNG image creation
TEST_F(ImageWriterTest, WritePNGImage) {
    ImageWriter writer("test_output.png", IMAGETYPE_PNG, width, height, north,
                       south, east, west);

    // Write a simple gradient pattern
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // Create RGBA gradient
            Pixel pixel = (255 << 24) |                        // Alpha
                          ((x * 255 / width) << 16) |          // Blue
                          ((y * 255 / height) << 8) |          // Green
                          ((x + y) * 128 / (width + height));  // Red
            writer.AppendPixel(pixel);
        }
        writer.EmitLine();
    }

    writer.Finish();

    // Verify file was created
    std::ifstream file("test_output.png");
    EXPECT_TRUE(file.good()) << "PNG output file should exist";
    file.close();
}

// Test PNG with solid colors
TEST_F(ImageWriterTest, PNGSolidColors) {
    ImageWriter writer("test_output.png", IMAGETYPE_PNG, 10, 10, north, south,
                       east, west);

    // Write solid red image
    for (int y = 0; y < 10; y++) {
        for (int x = 0; x < 10; x++) {
            Pixel pixel = 0xFF0000FF;  // Red with full alpha
            writer.AppendPixel(pixel);
        }
        writer.EmitLine();
    }

    writer.Finish();

    std::ifstream file("test_output.png");
    EXPECT_TRUE(file.good());
    file.close();
}
#endif

#ifdef HAVE_LIBJPEG
// Test JPEG image creation
TEST_F(ImageWriterTest, WriteJPEGImage) {
    ImageWriter writer("test_output.jpg", IMAGETYPE_JPG, width, height, north,
                       south, east, west);

    // Write a gradient pattern
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            Pixel pixel = (255 << 24) | ((x * 255 / width) << 16) |
                          ((y * 255 / height) << 8) | 128;
            writer.AppendPixel(pixel);
        }
        writer.EmitLine();
    }

    writer.Finish();

    // Verify file was created
    std::ifstream file("test_output.jpg");
    EXPECT_TRUE(file.good()) << "JPEG output file should exist";
    file.close();
}

// Test JPEG with different quality patterns
TEST_F(ImageWriterTest, JPEGComplexPattern) {
    ImageWriter writer("test_output.jpg", IMAGETYPE_JPG, 50, 50, north, south,
                       east, west);

    // Create a checkerboard pattern
    for (int y = 0; y < 50; y++) {
        for (int x = 0; x < 50; x++) {
            bool isBlack = ((x / 5) + (y / 5)) % 2 == 0;
            Pixel pixel = isBlack ? 0xFF000000 : 0xFFFFFFFF;
            writer.AppendPixel(pixel);
        }
        writer.EmitLine();
    }

    writer.Finish();

    std::ifstream file("test_output.jpg");
    EXPECT_TRUE(file.good());
    file.close();
}
#endif

// Test pixel color component extraction
TEST_F(ImageWriterTest, PixelColorComponents) {
    // Test pixel with specific RGB values
    // Pixel format is: 0xBBGGRRSS (B=Alpha/Blue, G=Green, R=Red, S=Red)
    Pixel pixel = 0xFF804020;  // B=255, G=128, R=64, S=32

    EXPECT_EQ(GetRValue(pixel), 64);   // R is bits 8-15
    EXPECT_EQ(GetGValue(pixel), 128);  // G is bits 16-23
    EXPECT_EQ(GetBValue(pixel), 255);  // B is bits 24-31 (alpha)
    EXPECT_EQ(GetSValue(pixel), 32);   // S is bits 0-7
}

// Test writing partial lines (edge case)
TEST_F(ImageWriterTest, PartialLineHandling) {
    ImageWriter writer("test_output.ppm", IMAGETYPE_PPM, 10, 5, north, south,
                       east, west);

    // Write only half a line (should be handled gracefully)
    for (int x = 0; x < 5; x++) {
        writer.AppendPixel(0xFFFFFFFF);
    }
    writer.EmitLine();

    // Complete the remaining lines normally
    for (int y = 1; y < 5; y++) {
        for (int x = 0; x < 10; x++) {
            writer.AppendPixel(0xFFFFFFFF);
        }
        writer.EmitLine();
    }

    writer.Finish();

    std::ifstream file("test_output.ppm");
    EXPECT_TRUE(file.good());
    file.close();
}

// Test geographic bounds
TEST_F(ImageWriterTest, GeographicBounds) {
    double testNorth = 45.5;
    double testSouth = 44.5;
    double testEast = -122.5;
    double testWest = -123.5;

    ImageWriter writer("test_output.ppm", IMAGETYPE_PPM, 50, 50, testNorth,
                       testSouth, testEast, testWest);

    EXPECT_EQ(writer.m_north, testNorth);
    EXPECT_EQ(writer.m_south, testSouth);
    EXPECT_EQ(writer.m_east, testEast);
    EXPECT_EQ(writer.m_west, testWest);

    // Write a simple image
    for (int y = 0; y < 50; y++) {
        for (int x = 0; x < 50; x++) {
            writer.AppendPixel(0xFFFFFFFF);
        }
        writer.EmitLine();
    }

    writer.Finish();
}

// Test multiple EmitLine calls track line numbers
TEST_F(ImageWriterTest, LineNumberTracking) {
    ImageWriter writer("test_output.ppm", IMAGETYPE_PPM, 10, 10, north, south,
                       east, west);

    EXPECT_EQ(writer.m_linenumber, 0);

    // Emit a few lines
    for (int line = 0; line < 3; line++) {
        for (int x = 0; x < 10; x++) {
            writer.AppendPixel(0xFFFFFFFF);
        }
        writer.EmitLine();
    }

    EXPECT_EQ(writer.m_linenumber, 3);

    writer.Finish();
}

// Test pixel offset tracking
TEST_F(ImageWriterTest, PixelOffsetTracking) {
    ImageWriter writer("test_output.ppm", IMAGETYPE_PPM, 10, 10, north, south,
                       east, west);

    EXPECT_EQ(writer.m_xoffset, 0);
    EXPECT_EQ(writer.m_xoffset_rgb, 0);

    // Add pixels
    for (int x = 0; x < 5; x++) {
        writer.AppendPixel(0xFFFFFFFF);
    }

    // After EmitLine, offsets should reset
    writer.EmitLine();
    EXPECT_EQ(writer.m_xoffset, 0);
    EXPECT_EQ(writer.m_xoffset_rgb, 0);

    writer.Finish();
}

// Parameterized test for different image formats
class ImageWriterFormatTest : public ImageWriterTest,
                              public ::testing::WithParamInterface<ImageType> {
  protected:
    void SetUp() override {
        ImageWriterTest::SetUp();
        imageType = GetParam();
    }

    std::string GetExtension() const {
        switch (imageType) {
        case IMAGETYPE_PPM:
            return ".ppm";
        case IMAGETYPE_PNG:
            return ".png";
        case IMAGETYPE_JPG:
            return ".jpg";
        case IMAGETYPE_GEOTIFF:
            return ".tif";
        default:
            return ".ppm";
        }
    }

    ImageType imageType;
};

// Test all supported image formats
TEST_P(ImageWriterFormatTest, WriteImageInFormat) {
    ImageType type = GetParam();
    std::string filename = "test_output" + GetExtension();

#ifndef HAVE_LIBPNG
    if (type == IMAGETYPE_PNG) {
        GTEST_SKIP() << "PNG support not compiled in";
    }
#endif
#ifndef HAVE_LIBJPEG
    if (type == IMAGETYPE_JPG) {
        GTEST_SKIP() << "JPEG support not compiled in";
    }
#endif
#ifndef HAVE_LIBGDAL
    if (type == IMAGETYPE_GEOTIFF) {
        GTEST_SKIP() << "GDAL/GeoTIFF support not compiled in";
    }
#endif

    EXPECT_NO_THROW({
        ImageWriter writer(filename, type, 50, 50, north, south, east, west);

        // Write a simple gradient
        for (int y = 0; y < 50; y++) {
            for (int x = 0; x < 50; x++) {
                Pixel pixel =
                    (255 << 24) | ((x * 5) << 16) | ((y * 5) << 8) | 128;
                writer.AppendPixel(pixel);
            }
            writer.EmitLine();
        }

        writer.Finish();

        // Verify file was created
        std::ifstream file(filename);
        EXPECT_TRUE(file.good())
            << "Output file should exist for format " << type;
        file.close();
    });

    std::remove(filename.c_str());
}

// Instantiate format tests
INSTANTIATE_TEST_SUITE_P(ImageFormats, ImageWriterFormatTest,
                         ::testing::Values(IMAGETYPE_PPM, IMAGETYPE_PNG,
                                           IMAGETYPE_JPG, IMAGETYPE_GEOTIFF));

// Test boundary coordinates (edge cases)
TEST_F(ImageWriterTest, BoundaryCoordinates) {
    // Test with extreme coordinate values
    ImageWriter writer("test_output.ppm", IMAGETYPE_PPM, 10, 10,
                       90.0,   // North (max latitude)
                       -90.0,  // South (min latitude)
                       180.0,  // East (max longitude)
                       -180.0  // West (min longitude)
    );

    EXPECT_EQ(writer.m_north, 90.0);
    EXPECT_EQ(writer.m_south, -90.0);
    EXPECT_EQ(writer.m_east, 180.0);
    EXPECT_EQ(writer.m_west, -180.0);

    // Write simple image
    for (int y = 0; y < 10; y++) {
        for (int x = 0; x < 10; x++) {
            writer.AppendPixel(0xFFFFFFFF);
        }
        writer.EmitLine();
    }

    writer.Finish();
}

// Test alpha channel values
TEST_F(ImageWriterTest, AlphaChannelValues) {
    ImageWriter writer("test_output.ppm", IMAGETYPE_PPM, 10, 10, north, south,
                       east, west);

    // Test various alpha values
    std::vector<Pixel> alphaValues = {
        0x00FFFFFF,  // Transparent white
        0x80FFFFFF,  // Semi-transparent white
        0xFFFFFFFF,  // Opaque white
        0xFF000000,  // Opaque black
        0x00000000   // Transparent black
    };

    for (const auto &pixel : alphaValues) {
        writer.AppendPixel(pixel);
    }
    writer.EmitLine();

    writer.Finish();
}

// Test writing maximum size line
TEST_F(ImageWriterTest, MaximumWidthLine) {
    int largeWidth = 4096;  // Common max texture size
    ImageWriter writer("test_output.ppm", IMAGETYPE_PPM, largeWidth, 1, north,
                       south, east, west);

    // Write a single line with maximum width
    for (int x = 0; x < largeWidth; x++) {
        writer.AppendPixel(0xFFFFFFFF);
    }
    writer.EmitLine();

    writer.Finish();

    std::ifstream file("test_output.ppm");
    EXPECT_TRUE(file.good());
    file.close();
}
