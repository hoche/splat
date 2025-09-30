#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "splat.h"

// Mock class for libpng/libjpeg dependencies
class MockImageWriter {
public:
    MOCK_METHOD3(write_png, int(const char* filename, unsigned char* image, int width));
    MOCK_METHOD3(write_jpeg, int(const char* filename, unsigned char* image, int width));
    MOCK_METHOD3(write_ppm, int(const char* filename, unsigned char* image, int width));
};

// Test fixture for image output functions
class ImageOutputTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize common test data
        width = 100;
        height = 100;
        image_data = new unsigned char[width * height * 3]; // RGB image
        for (int i = 0; i < width * height * 3; ++i) {
            image_data[i] = static_cast<unsigned char>(i % 256); // Sample pixel data
        }
    }

    void TearDown() override {
        delete[] image_data;
    }

    unsigned char* image_data;
    int width;
    int height;
    MockImageWriter mock_writer;
};

// Test WriteImage for PNG output
TEST_F(ImageOutputTest, WriteImagePNGOutput) {
    // Mock the PNG writing function
    EXPECT_CALL(mock_writer, write_png(testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(0));

    // Call WriteImage with PNG format (default)
    int result = WriteImage("test.png", image_data, width, height); // Adjust parameters as needed
    EXPECT_EQ(result, 0) << "WriteImage should succeed for PNG output";
}

// Test WriteImage for JPEG output
TEST_F(ImageOutputTest, WriteImageJPEGOutput) {
    // Mock the JPEG writing function
    EXPECT_CALL(mock_writer, write_jpeg(testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(0));

    // Call WriteImage with JPEG format
    int result = WriteImage("test.jpg", image_data, width, height); // Adjust parameters as needed
    EXPECT_EQ(result, 0) << "WriteImage should succeed for JPEG output";
}

// Test WriteImage for PPM output
TEST_F(ImageOutputTest, WriteImagePPMOutput) {
    // Mock the PPM writing function
    EXPECT_CALL(mock_writer, write_ppm(testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(0));

    // Call WriteImage with PPM format
    int result = WriteImage("test.ppm", image_data, width, height); // Adjust parameters as needed
    EXPECT_EQ(result, 0) << "WriteImage should succeed for PPM output";
}

// Test WriteImage with invalid filename
TEST_F(ImageOutputTest, WriteImageInvalidFilename) {
    // Mock the PNG writing function to simulate failure
    EXPECT_CALL(mock_writer, write_png(testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(-1));

    // Call WriteImage with an invalid filename
    int result = WriteImage("", image_data, width, height); // Empty filename
    EXPECT_NE(result, 0) << "WriteImage should fail with invalid filename";
}

// Test WriteImageSS for PNG output
TEST_F(ImageOutputTest, WriteImageSSPNGOutput) {
    // Mock the PNG writing function
    EXPECT_CALL(mock_writer, write_png(testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(0));

    // Call WriteImageSS with PNG format
    int result = WriteImageSS("test_ss.png", image_data, width, height); // Adjust parameters
    EXPECT_EQ(result, 0) << "WriteImageSS should succeed for PNG output";
}

// Parameterized test for different image sizes
class ImageOutputSizeTest : public ImageOutputTest, public ::testing::WithParamInterface<std::tuple<int, int>> {
protected:
    void SetUp() override {
        ImageOutputTest::SetUp();
        width = std::get<0>(GetParam());
        height = std::get<1>(GetParam());
        delete[] image_data; // Clean up base class data
        image_data = new unsigned char[width * height * 3]; // Resize image data
        for (int i = 0; i < width * height * 3; ++i) {
            image_data[i] = static_cast<unsigned char>(i % 256);
        }
    }
};

// Test WriteImage with various image sizes
TEST_P(ImageOutputSizeTest, WriteImageVariousSizes) {
    // Mock the PNG writing function
    EXPECT_CALL(mock_writer, write_png(testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(0));

    // Call WriteImage with varying sizes
    int result = WriteImage("test_size.png", image_data, width, height);
    EXPECT_EQ(result, 0) << "WriteImage should succeed for size " << width << "x" << height;
}

// Instantiate parameterized test with different image sizes
INSTANTIATE_TEST_SUITE_P(
    ImageSizes,
    ImageOutputSizeTest,
    ::testing::Values(
        std::make_tuple(1, 1),    // Minimal size
        std::make_tuple(100, 100), // Standard size
        std::make_tuple(1920, 1080) // HD size
    ));

// Test for null image data
TEST_F(ImageOutputTest, WriteImageNullData) {
    // Mock the PNG writing function (should not be called)
    EXPECT_CALL(mock_writer, write_png(testing::_, testing::_, testing::_)).Times(0);

    // Call WriteImage with null image data
    int result = WriteImage("test_null.png", nullptr, width, height);
    EXPECT_NE(result, 0) << "WriteImage should fail with null image data";
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
