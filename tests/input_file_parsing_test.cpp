#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>
#include <vector>

// Mock class for file I/O dependencies
class MockFileIO {
public:
    MOCK_METHOD2(read_file, bool(const std::string& filename, std::vector<std::string>& lines));
    MOCK_METHOD1(read_binary_file, bool(const std::string& filename));
};

// Test fixture for input file parsing
class InputFileParsingTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize common test data
        sdf_data.clear();
        epsilon = 1e-6; // Tolerance for floating-point comparisons
    }

    std::vector<std::vector<float>> sdf_data; // Simulated storage for SDF elevation data
    double epsilon;
    MockFileIO mock_file_io;
};

// Mock function to simulate ParseSDF (adjust signature as needed)
// Parses an SDF file into a 2D vector of elevation data
bool ParseSDF(const std::string& filename, std::vector<std::vector<float>>& elevation_data, MockFileIO& file_io) {
    std::vector<std::string> lines;
    if (!file_io.read_file(filename, lines)) {
        return false;
    }
    elevation_data.clear();
    try {
        // Assume SDF file contains a grid of elevation values (one per line, space-separated)
        for (const auto& line : lines) {
            std::vector<float> row;
            std::istringstream iss(line);
            float value;
            while (iss >> value) {
                row.push_back(value);
            }
            if (!row.empty()) {
                elevation_data.push_back(row);
            }
        }
        return !elevation_data.empty();
    } catch (...) {
        return false;
    }
}

// Test parsing a valid SDF file
TEST_F(InputFileParsingTest, ParseValidSDF) {
    // Mock file content for a valid SDF file (3x3 grid of elevation values)
    std::vector<std::string> mock_lines = {
        "100.0 150.0 200.0",
        "120.0 130.0 140.0",
        "110.0 160.0 180.0"
    };
    EXPECT_CALL(mock_file_io, read_file("46_47_122_123.sdf", testing::_))
        .WillOnce(testing::DoAll(
            testing::SetArgReferee<1>(mock_lines),
            testing::Return(true)));

    // Call ParseSDF
    bool result = ParseSDF("46_47_122_123.sdf", sdf_data, mock_file_io);
    EXPECT_TRUE(result) << "Parsing valid SDF file should succeed";
    ASSERT_EQ(sdf_data.size(), 3) << "SDF data should have 3 rows";
    ASSERT_EQ(sdf_data[0].size(), 3) << "Each row should have 3 columns";
    EXPECT_NEAR(sdf_data[0][0], 100.0, epsilon) << "First row, first value should be 100.0";
    EXPECT_NEAR(sdf_data[1][1], 130.0, epsilon) << "Second row, second value should be 130.0";
    EXPECT_NEAR(sdf_data[2][2], 180.0, epsilon) << "Third row, third value should be 180.0";
}

// Test parsing an empty SDF file
TEST_F(InputFileParsingTest, ParseEmptySDF) {
    // Mock empty file
    std::vector<std::string> mock_lines = {};
    EXPECT_CALL(mock_file_io, read_file("empty.sdf", testing::_))
        .WillOnce(testing::DoAll(
            testing::SetArgReferee<1>(mock_lines),
            testing::Return(true)));

    // Call ParseSDF
    bool result = ParseSDF("empty.sdf", sdf_data, mock_file_io);
    EXPECT_FALSE(result) << "Parsing empty SDF file should fail";
    EXPECT_TRUE(sdf_data.empty()) << "SDF data should remain empty";
}

// Test parsing an SDF file with invalid data
TEST_F(InputFileParsingTest, ParseInvalidSDF) {
    // Mock file with invalid data
    std::vector<std::string> mock_lines = {
        "100.0 150.0 200.0",
        "120.0 invalid 140.0",
        "110.0 160.0 180.0"
    };
    EXPECT_CALL(mock_file_io, read_file("invalid.sdf", testing::_))
        .WillOnce(testing::DoAll(
            testing::SetArgReferee<1>(mock_lines),
            testing::Return(true)));

    // Call ParseSDF
    bool result = ParseSDF("invalid.sdf", sdf_data, mock_file_io);
    EXPECT_FALSE(result) << "Parsing SDF file with invalid data should fail";
    EXPECT_TRUE(sdf_data.empty()) << "SDF data should remain empty on failure";
}

// Test parsing a missing SDF file
TEST_F(InputFileParsingTest, ParseMissingSDF) {
    // Mock file read failure
    EXPECT_CALL(mock_file_io, read_file("missing.sdf", testing::_))
        .WillOnce(testing::Return(false));

    // Call ParseSDF
    bool result = ParseSDF("missing.sdf", sdf_data, mock_file_io);
    EXPECT_FALSE(result) << "Parsing should fail if SDF file is missing";
    EXPECT_TRUE(sdf_data.empty()) << "SDF data should remain empty on failure";
}

// Test parsing SDF with inconsistent row lengths
TEST_F(InputFileParsingTest, ParseInconsistentSDF) {
    // Mock file with inconsistent row lengths
    std::vector<std::string> mock_lines = {
        "100.0 150.0 200.0",
        "120.0 130.0",
        "110.0 160.0 180.0"
    };
    EXPECT_CALL(mock_file_io, read_file("inconsistent.sdf", testing::_))
        .WillOnce(testing::DoAll(
            testing::SetArgReferee<1>(mock_lines),
            testing::Return(true)));

    // Call ParseSDF
    bool result = ParseSDF("inconsistent.sdf", sdf_data, mock_file_io);
    EXPECT_TRUE(result) << "Parsing SDF with inconsistent rows should succeed but store valid rows";
    ASSERT_EQ(sdf_data.size(), 3) << "SDF data should have 3 rows";
    EXPECT_EQ(sdf_data[1].size(), 2) << "Second row should have 2 columns";
    EXPECT_NEAR(sdf_data[1][0], 120.0, epsilon) << "Second row, first value should be 120.0";
    EXPECT_NEAR(sdf_data[1][1], 130.0, epsilon) << "Second row, second value should be 130.0";
}

// Test parsing SDF with whitespace variations
TEST_F(InputFileParsingTest, ParseSDFWithWhitespace) {
    // Mock file with extra whitespace
    std::vector<std::string> mock_lines = {
        "  100.0   150.0  200.0  ",
        "\t120.0 130.0\t140.0",
        "110.0\t 160.0  180.0 "
    };
    EXPECT_CALL(mock_file_io, read_file("whitespace.sdf", testing::_))
        .WillOnce(testing::DoAll(
            testing::SetArgReferee<1>(mock_lines),
            testing::Return(true)));

    // Call ParseSDF
    bool result = ParseSDF("whitespace.sdf", sdf_data, mock_file_io);
    EXPECT_TRUE(result) << "Parsing SDF with whitespace variations should succeed";
    ASSERT_EQ(sdf_data.size(), 3) << "SDF data should have 3 rows";
    ASSERT_EQ(sdf_data[0].size(), 3) << "Each row should have 3 columns";
    EXPECT_NEAR(sdf_data[0][0], 100.0, epsilon) << "First row, first value should be 100.0";
    EXPECT_NEAR(sdf_data[1][1], 130.0, epsilon) << "Second row, second value should be 130.0";
}

// Parameterized test for different SDF grid sizes
class SDFGridSizeTest : public InputFileParsingTest, public ::testing::WithParamInterface<std::tuple<int, int>> {
protected:
    void SetUp() override {
        InputFileParsingTest::SetUp();
        int rows = std::get<0>(GetParam());
        int cols = std::get<1>(GetParam());
        mock_lines.clear();
        for (int i = 0; i < rows; ++i) {
            std::string line;
            for (int j = 0; j < cols; ++j) {
                line += std::to_string(100.0 + i * cols + j) + " ";
            }
            mock_lines.push_back(line);
        }
    }

    std::vector<std::string> mock_lines;
};

// Test parsing SDF with various grid sizes
TEST_P(SDFGridSizeTest, ParseVariousGridSizes) {
    EXPECT_CALL(mock_file_io, read_file("grid.sdf", testing::_))
        .WillOnce(testing::DoAll(
            testing::SetArgReferee<1>(mock_lines),
            testing::Return(true)));

    // Call ParseSDF
    bool result = ParseSDF("grid.sdf", sdf_data, mock_file_io);
    EXPECT_TRUE(result) << "Parsing SDF with grid size " << std::get<0>(GetParam()) << "x" << std::get<1>(GetParam()) << " should succeed";
    EXPECT_EQ(sdf_data.size(), static_cast<size_t>(std::get<0>(GetParam()))) << "SDF data should have correct number of rows";
    if (!sdf_data.empty()) {
        EXPECT_EQ(sdf_data[0].size(), static_cast<size_t>(std::get<1>(GetParam()))) << "SDF data should have correct number of columns";
        for (size_t i = 0; i < sdf_data.size(); ++i) {
            for (size_t j = 0; j < sdf_data[i].size(); ++j) {
                EXPECT_NEAR(sdf_data[i][j], 100.0 + i * std::get<1>(GetParam()) + j, epsilon)
                    << "Value at (" << i << "," << j << ") should match expected";
            }
        }
    }
}

// Instantiate parameterized test with different grid sizes
INSTANTIATE_TEST_SUITE_P(
    GridSizes,
    SDFGridSizeTest,
    ::testing::Values(
        std::make_tuple(1, 1),    // 1x1 grid
        std::make_tuple(3, 3),    // 3x3 grid
        std::make_tuple(10, 5),   // 10x5 grid
        std::make_tuple(8, 8)     // 8x8 grid (max supported per repository)
    ));

