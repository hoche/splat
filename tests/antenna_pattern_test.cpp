#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>
#include <vector>

#include "splat.h"

// Mock class for file I/O dependencies
class MockFileIO {
public:
    MOCK_METHOD2(read_file, bool(const std::string& filename, std::vector<std::string>& lines));
};

// Test fixture for antenna pattern parsing
class AntennaPatternTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize common test data
        azimuth_pattern.clear();
        elevation_pattern.clear();
    }

    std::vector<float> azimuth_pattern;   // Simulated storage for .az file data
    std::vector<float> elevation_pattern; // Simulated storage for .el file data
    MockFileIO mock_file_io;
};

// Mock function to simulate ReadAntennaPattern (adjust signature as needed)
bool ReadAntennaPattern(const std::string& filename, std::vector<float>& pattern, MockFileIO& file_io) {
    std::vector<std::string> lines;
    if (!file_io.read_file(filename, lines)) {
        return false;
    }
    pattern.clear();
    for (const auto& line : lines) {
        try {
            float value = std::stof(line);
            pattern.push_back(value);
        } catch (...) {
            return false;
        }
    }
    return true;
}

// Test parsing a valid azimuth pattern file
TEST_F(AntennaPatternTest, ParseValidAzimuthPattern) {
    // Mock file content for a valid .az file
    std::vector<std::string> mock_lines = {"0.0", "1.5", "3.2", "0.0"}; // Includes zero for perfect blocking
    EXPECT_CALL(mock_file_io, read_file("test.az", testing::_))
        .WillOnce(testing::DoAll(
            testing::SetArgReferee<1>(mock_lines),
            testing::Return(true)));

    // Call the antenna pattern parsing function
    bool result = ReadAntennaPattern("test.az", azimuth_pattern, mock_file_io);
    EXPECT_TRUE(result) << "Parsing valid azimuth pattern should succeed";
    ASSERT_EQ(azimuth_pattern.size(), 4) << "Azimuth pattern should have 4 values";
    EXPECT_FLOAT_EQ(azimuth_pattern[0], 0.0f) << "First value should be 0.0 (perfect blocking)";
    EXPECT_FLOAT_EQ(azimuth_pattern[1], 1.5f) << "Second value should be 1.5";
    EXPECT_FLOAT_EQ(azimuth_pattern[2], 3.2f) << "Third value should be 3.2";
    EXPECT_FLOAT_EQ(azimuth_pattern[3], 0.0f) << "Fourth value should be 0.0";
}

// Test parsing a valid elevation pattern file
TEST_F(AntennaPatternTest, ParseValidElevationPattern) {
    // Mock file content for a valid .el file
    std::vector<std::string> mock_lines = {"2.0", "-1.0", "0.0"}; // Includes zero and negative value
    EXPECT_CALL(mock_file_io, read_file("test.el", testing::_))
        .WillOnce(testing::DoAll(
            testing::SetArgReferee<1>(mock_lines),
            testing::Return(true)));

    // Call the antenna pattern parsing function
    bool result = ReadAntennaPattern("test.el", elevation_pattern, mock_file_io);
    EXPECT_TRUE(result) << "Parsing valid elevation pattern should succeed";
    ASSERT_EQ(elevation_pattern.size(), 3) << "Elevation pattern should have 3 values";
    EXPECT_FLOAT_EQ(elevation_pattern[0], 2.0f) << "First value should be 2.0";
    EXPECT_FLOAT_EQ(elevation_pattern[1], -1.0f) << "Second value should be -1.0";
    EXPECT_FLOAT_EQ(elevation_pattern[2], 0.0f) << "Third value should be 0.0 (perfect blocking)";
}

// Test parsing an empty file
TEST_F(AntennaPatternTest, ParseEmptyFile) {
    // Mock empty file
    std::vector<std::string> mock_lines = {};
    EXPECT_CALL(mock_file_io, read_file("empty.az", testing::_))
        .WillOnce(testing::DoAll(
            testing::SetArgReferee<1>(mock_lines),
            testing::Return(true)));

    // Call the antenna pattern parsing function
    bool result = ReadAntennaPattern("empty.az", azimuth_pattern, mock_file_io);
    EXPECT_TRUE(result) << "Parsing empty file should succeed (empty pattern)";
    EXPECT_TRUE(azimuth_pattern.empty()) << "Azimuth pattern should be empty";
}

// Test parsing a file with invalid data
TEST_F(AntennaPatternTest, ParseInvalidData) {
    // Mock file with invalid data
    std::vector<std::string> mock_lines = {"1.0", "invalid", "3.0"};
    EXPECT_CALL(mock_file_io, read_file("invalid.az", testing::_))
        .WillOnce(testing::DoAll(
            testing::SetArgReferee<1>(mock_lines),
            testing::Return(true)));

    // Call the antenna pattern parsing function
    bool result = ReadAntennaPattern("invalid.az", azimuth_pattern, mock_file_io);
    EXPECT_FALSE(result) << "Parsing file with invalid data should fail";
    EXPECT_TRUE(azimuth_pattern.empty()) << "Azimuth pattern should remain empty on failure";
}

// Test file read failure
TEST_F(AntennaPatternTest, FileReadFailure) {
    // Mock file read failure
    EXPECT_CALL(mock_file_io, read_file("missing.az", testing::_))
        .WillOnce(testing::Return(false));

    // Call the antenna pattern parsing function
    bool result = ReadAntennaPattern("missing.az", azimuth_pattern, mock_file_io);
    EXPECT_FALSE(result) << "Parsing should fail if file read fails";
    EXPECT_TRUE(azimuth_pattern.empty()) << "Azimuth pattern should remain empty on failure";
}

// Parameterized test for various pattern sizes
class AntennaPatternSizeTest : public AntennaPatternTest, public ::testing::WithParamInterface<int> {
protected:
    void SetUp() override {
        AntennaPatternTest::SetUp();
        int size = GetParam();
        mock_lines.resize(size);
        for (int i = 0; i < size; ++i) {
            mock_lines[i] = std::to_string(static_cast<float>(i) / 10.0f);
        }
    }

    std::vector<std::string> mock_lines;
};

// Test parsing with different pattern sizes
TEST_P(AntennaPatternSizeTest, ParseVariousSizes) {
    EXPECT_CALL(mock_file_io, read_file("test.az", testing::_))
        .WillOnce(testing::DoAll(
            testing::SetArgReferee<1>(mock_lines),
            testing::Return(true)));

    // Call the antenna pattern parsing function
    bool result = ReadAntennaPattern("test.az", azimuth_pattern, mock_file_io);
    EXPECT_TRUE(result) << "Parsing pattern of size " << GetParam() << " should succeed";
    EXPECT_EQ(azimuth_pattern.size(), static_cast<size_t>(GetParam())) << "Pattern size should match input";
    for (int i = 0; i < GetParam(); ++i) {
        EXPECT_FLOAT_EQ(azimuth_pattern[i], static_cast<float>(i) / 10.0f)
            << "Pattern value at index " << i << " should match input";
    }
}

// Instantiate parameterized test with different pattern sizes
INSTANTIATE_TEST_SUITE_P(
    PatternSizes,
    AntennaPatternSizeTest,
    ::testing::Values(0, 1, 10, 360)); // Empty, single value, small pattern, full 360-degree pattern

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
