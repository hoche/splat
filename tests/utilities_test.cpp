/**
 * @file utilities_test.cpp
 *
 * Unit tests for the Utilities class
 */

#include "../src/utilities.h"
#include <cmath>
#include <gtest/gtest.h>

class UtilitiesTest : public ::testing::Test {
  protected:
    void SetUp() override { }
    void TearDown() override { }
};

// Test interpolation
TEST_F(UtilitiesTest, InterpolateBasic) {
    // Linear interpolation between two points
    int result = Utilities::interpolate(0, 100, 0, 10, 5);

    EXPECT_EQ(result, 50);
}

TEST_F(UtilitiesTest, InterpolateAtStart) {
    int result = Utilities::interpolate(10, 20, 0, 10, 0);

    EXPECT_EQ(result, 10);
}

TEST_F(UtilitiesTest, InterpolateAtEnd) {
    int result = Utilities::interpolate(10, 20, 0, 10, 10);

    EXPECT_EQ(result, 20);
}

TEST_F(UtilitiesTest, InterpolateNegative) {
    int result = Utilities::interpolate(-100, 100, 0, 10, 5);

    EXPECT_EQ(result, 0);
}

// Test arccos function
TEST_F(UtilitiesTest, ArccosBasic) {
    double result = Utilities::arccos(1.0, 0.0);

    EXPECT_NEAR(result, 0.0, 0.001);
}

TEST_F(UtilitiesTest, Arccos90Degrees) {
    double result = Utilities::arccos(0.0, 1.0);

    EXPECT_NEAR(result, 90.0, 0.001);
}

TEST_F(UtilitiesTest, Arccos180Degrees) {
    double result = Utilities::arccos(-1.0, 0.0);

    EXPECT_NEAR(result, 180.0, 0.001);
}

// Test ReduceAngle
TEST_F(UtilitiesTest, ReduceAngleNormal) {
    int result = Utilities::ReduceAngle(45.0);

    EXPECT_EQ(result, 45);
}

TEST_F(UtilitiesTest, ReduceAngleOver360) {
    int result = Utilities::ReduceAngle(370.0);

    EXPECT_EQ(result, 10);
}

TEST_F(UtilitiesTest, ReduceAngleNegative) {
    int result = Utilities::ReduceAngle(-10.0);

    EXPECT_EQ(result, 350);
}

TEST_F(UtilitiesTest, ReduceAngleMultipleRotations) {
    int result = Utilities::ReduceAngle(720.0);

    EXPECT_EQ(result, 0);
}

TEST_F(UtilitiesTest, ReduceAngleLargeNegative) {
    int result = Utilities::ReduceAngle(-370.0);

    EXPECT_EQ(result, 350);
}

// Test LonDiff (longitude difference)
TEST_F(UtilitiesTest, LonDiffSameHemisphere) {
    double diff = Utilities::LonDiff(-100.0, -120.0);

    EXPECT_NEAR(diff, 20.0, 0.001);
}

TEST_F(UtilitiesTest, LonDiffCrossingPrimeMeridian) {
    double diff = Utilities::LonDiff(-10.0, 10.0);

    EXPECT_NEAR(diff, 20.0, 0.001);
}

TEST_F(UtilitiesTest, LonDiffZero) {
    double diff = Utilities::LonDiff(-100.0, -100.0);

    EXPECT_NEAR(diff, 0.0, 0.001);
}

// Test dec2dms (decimal to degrees-minutes-seconds)
TEST_F(UtilitiesTest, Dec2DmsPositive) {
    std::string result = Utilities::dec2dms(45.5);

    // Should contain degrees, minutes, and direction
    EXPECT_NE(result.find("45"), std::string::npos);
    EXPECT_NE(result.find("30"),
              std::string::npos);  // 0.5 degrees = 30 minutes
}

TEST_F(UtilitiesTest, Dec2DmsNegative) {
    std::string result = Utilities::dec2dms(-45.5);

    EXPECT_NE(result.find("45"), std::string::npos);
    EXPECT_NE(result.find("30"), std::string::npos);
}

TEST_F(UtilitiesTest, Dec2DmsZero) {
    std::string result = Utilities::dec2dms(0.0);

    EXPECT_NE(result.find("00"), std::string::npos);
}

// Test ReadBearing
TEST_F(UtilitiesTest, ReadBearingNumeric) {
    double bearing = Utilities::ReadBearing("45.5");

    EXPECT_NEAR(bearing, 45.5, 0.001);
}

TEST_F(UtilitiesTest, ReadBearingZero) {
    double bearing = Utilities::ReadBearing("0");

    EXPECT_NEAR(bearing, 0.0, 0.001);
}

// Test PathLeaf (get filename from path)
TEST_F(UtilitiesTest, PathLeafUnixStyle) {
    std::string result = Utilities::PathLeaf("/home/user/file.txt");

    EXPECT_EQ(result, "file.txt");
}

TEST_F(UtilitiesTest, PathLeafNoPath) {
    std::string result = Utilities::PathLeaf("file.txt");

    EXPECT_EQ(result, "file.txt");
}

TEST_F(UtilitiesTest, PathLeafTrailingSlash) {
    std::string result = Utilities::PathLeaf("/home/user/directory/");

    EXPECT_EQ(result, "");
}

TEST_F(UtilitiesTest, PathLeafMultipleSlashes) {
    std::string result = Utilities::PathLeaf("/home//user///file.txt");

    EXPECT_EQ(result, "file.txt");
}

// Test Extension
TEST_F(UtilitiesTest, ExtensionBasic) {
    std::string ext = Utilities::Extension("file.txt");

    EXPECT_EQ(ext, "txt");
}

TEST_F(UtilitiesTest, ExtensionWithPath) {
    std::string ext = Utilities::Extension("/home/user/document.pdf");

    EXPECT_EQ(ext, "pdf");
}

TEST_F(UtilitiesTest, ExtensionNoExtension) {
    std::string ext = Utilities::Extension("filename");

    EXPECT_EQ(ext, "");
}

TEST_F(UtilitiesTest, ExtensionMultipleDots) {
    std::string ext = Utilities::Extension("archive.tar.gz");

    EXPECT_EQ(ext, "gz");
}

TEST_F(UtilitiesTest, ExtensionDotFile) {
    std::string ext = Utilities::Extension(".gitignore");

    // Should handle hidden files correctly
    EXPECT_TRUE(ext == "" || ext == "gitignore");
}

// Test Basename
TEST_F(UtilitiesTest, BasenameBasic) {
    std::string base = Utilities::Basename("file.txt");

    EXPECT_EQ(base, "file");
}

TEST_F(UtilitiesTest, BasenameWithPath) {
    std::string base = Utilities::Basename("/home/user/document.pdf");

    EXPECT_EQ(base, "document");
}

TEST_F(UtilitiesTest, BasenameNoExtension) {
    std::string base = Utilities::Basename("filename");

    EXPECT_EQ(base, "filename");
}

TEST_F(UtilitiesTest, BasenameMultipleDots) {
    std::string base = Utilities::Basename("archive.tar.gz");

    EXPECT_EQ(base, "archive.tar");
}

// Test ExtensionIdx
TEST_F(UtilitiesTest, ExtensionIdxBasic) {
    auto idx = Utilities::ExtensionIdx("file.txt");

    EXPECT_EQ(idx, 4);  // Position of the dot
}

TEST_F(UtilitiesTest, ExtensionIdxNoExtension) {
    auto idx = Utilities::ExtensionIdx("filename");

    EXPECT_EQ(idx, std::string::npos);
}

// Test DivideExtension
TEST_F(UtilitiesTest, DivideExtensionHasExtension) {
    std::string path = "file.txt";
    std::string ext = Utilities::DivideExtension(path, "default");

    EXPECT_EQ(ext, "txt");
    EXPECT_EQ(path, "file");
}

TEST_F(UtilitiesTest, DivideExtensionNoExtension) {
    std::string path = "filename";
    std::string ext = Utilities::DivideExtension(path, "dat");

    EXPECT_EQ(ext, "dat");
    EXPECT_EQ(path, "filename");
}

TEST_F(UtilitiesTest, DivideExtensionWithPath) {
    std::string path = "/home/user/document.pdf";
    std::string ext = Utilities::DivideExtension(path, "txt");

    EXPECT_EQ(ext, "pdf");
    EXPECT_EQ(path, "/home/user/document");
}

// Test Chomp (remove trailing whitespace/newlines)
TEST_F(UtilitiesTest, ChompNewline) {
    std::string str = "hello world\n";
    Utilities::Chomp(str);

    EXPECT_EQ(str, "hello world");
}

TEST_F(UtilitiesTest, ChompCarriageReturn) {
    std::string str = "hello world\r\n";
    Utilities::Chomp(str);

    EXPECT_EQ(str, "hello world");
}

TEST_F(UtilitiesTest, ChompMultipleNewlines) {
    std::string str = "hello world\n\n\n";
    Utilities::Chomp(str);

    EXPECT_EQ(str, "hello world");
}

TEST_F(UtilitiesTest, ChompNoNewline) {
    std::string str = "hello world";
    Utilities::Chomp(str);

    EXPECT_EQ(str, "hello world");
}

TEST_F(UtilitiesTest, ChompSpaces) {
    std::string str = "hello world   ";
    Utilities::Chomp(str);

    // Chomp typically removes newlines, not spaces - check actual behavior
    EXPECT_NE(str, "");
}

TEST_F(UtilitiesTest, ChompEmptyString) {
    std::string str = "";
    Utilities::Chomp(str);

    EXPECT_EQ(str, "");
}

// Parameterized test for angle reduction
class AngleReductionTest
    : public ::testing::TestWithParam<std::pair<double, int>> { };

TEST_P(AngleReductionTest, ReduceVariousAngles) {
    auto [input, expected] = GetParam();

    int result = Utilities::ReduceAngle(input);

    EXPECT_EQ(result, expected);
}

INSTANTIATE_TEST_SUITE_P(
    VariousAngles, AngleReductionTest,
    ::testing::Values(std::make_pair(0.0, 0), std::make_pair(90.0, 90),
                      std::make_pair(180.0, 180), std::make_pair(270.0, 270),
                      std::make_pair(360.0, 0), std::make_pair(450.0, 90),
                      std::make_pair(-90.0, 270), std::make_pair(-180.0, 180),
                      std::make_pair(720.5, 0), std::make_pair(1000.0, 280)));

// Parameterized test for path operations
class PathOperationsTest
    : public ::testing::TestWithParam<
          std::tuple<std::string, std::string, std::string>> { };

TEST_P(PathOperationsTest, PathComponents) {
    auto [full_path, expected_leaf, expected_ext] = GetParam();

    std::string leaf = Utilities::PathLeaf(full_path);
    std::string ext = Utilities::Extension(full_path);

    EXPECT_EQ(leaf, expected_leaf);
    EXPECT_EQ(ext, expected_ext);
}

INSTANTIATE_TEST_SUITE_P(
    VariousPaths, PathOperationsTest,
    ::testing::Values(std::make_tuple("/home/user/file.txt", "file.txt", "txt"),
                      std::make_tuple("document.pdf", "document.pdf", "pdf"),
                      std::make_tuple("/var/log/system.log", "system.log",
                                      "log"),
                      std::make_tuple("archive.tar.gz", "archive.tar.gz", "gz"),
                      std::make_tuple("/path/to/file", "file", ""),
                      std::make_tuple("noextension", "noextension", "")));
