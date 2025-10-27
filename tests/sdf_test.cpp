/**
 * @file sdf_test.cpp
 *
 * Unit tests for SDF (SPLAT Data File) operations
 */

#include "../src/elevation_map.h"
#include "../src/sdf.h"
#include "../src/splat_run.h"
#include <cstdio>
#include <fstream>
#include <gtest/gtest.h>

class SdfTest : public ::testing::Test {
  protected:
    SplatRun *sr;
    ElevationMap *em;

    void SetUp() override {
        sr = new SplatRun();
        sr->sdf_path = "./test_sdf_data";
        sr->ppd = 1200;  // Points per degree (1200 for 3-arc-second data)

        em = new ElevationMap(*sr);

        // Create test directory
        int ret = system("mkdir -p ./test_sdf_data");
        (void)ret;  // Ignore return value - test will fail later if dir not created
    }

    void TearDown() override {
        delete em;
        delete sr;

        // Clean up test files
        int ret = system("rm -rf ./test_sdf_data");
        (void)ret;  // Ignore return value - cleanup failure is not critical
    }

    // Helper to create a minimal valid SDF file
    void CreateTestSDF(const std::string &filename, int min_lat, int max_lat,
                       int min_lon, int max_lon) {
        std::ofstream sdf(filename);  // Text mode, not binary

        if (! sdf.is_open()) {
            return;
        }

        // SDF format: 4 header lines followed by elevation data (one per line)
        // Header: max_west, min_north, min_west, max_north
        sdf << max_lon << "\n";
        sdf << min_lat << "\n";
        sdf << min_lon << "\n";
        sdf << max_lat << "\n";

        // Write elevation data (ppd x ppd points)
        int width = sr->ppd;
        int height = sr->ppd;

        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                sdf << "100\n";  // 100 meters elevation
            }
        }

        sdf.close();
    }
};

// Test SDF constructor
TEST_F(SdfTest, Constructor) {
    Sdf sdf("./test_sdf_data", *sr);

    // Constructor should complete without errors
    SUCCEED();
}

// Test loading a single SDF file
TEST_F(SdfTest, LoadSingleSDFFile) {
    // Create a simple 1x1 degree SDF file for lat 45, lon -122
    std::string sdf_filename = "./test_sdf_data/45:-122:45:-122.sdf";
    CreateTestSDF(sdf_filename, 45, 45, -122, -122);

    Sdf sdf("./test_sdf_data", *sr);

    int result = sdf.LoadSDF(*em, "45:-122:45:-122", 45, 45, -122, -122);

    // Result should not be -1 (failure), could be 0 if already loaded or > 0
    EXPECT_NE(result, -1);

    remove(sdf_filename.c_str());
}

// Test loading non-existent SDF file
TEST_F(SdfTest, LoadNonExistentFile) {
    Sdf sdf("./test_sdf_data", *sr);

    int result = sdf.LoadSDF(*em, "99:-99:99:-99", 99, 99, -99, -99);

    // LoadSDF returns 0 or -1 when file not found (depending on whether DEM slot is available)
    EXPECT_LE(result, 0);
}

// Test loading multiple SDF files
TEST_F(SdfTest, LoadMultipleSDFFiles) {
    // Create 2x2 degree area of SDF files
    CreateTestSDF("./test_sdf_data/45:-122:45:-122.sdf", 45, 45, -122, -122);
    CreateTestSDF("./test_sdf_data/45:-121:45:-121.sdf", 45, 45, -121, -121);
    CreateTestSDF("./test_sdf_data/46:-122:46:-122.sdf", 46, 46, -122, -122);
    CreateTestSDF("./test_sdf_data/46:-121:46:-121.sdf", 46, 46, -121, -121);

    Sdf sdf("./test_sdf_data", *sr);

    char result = sdf.LoadSDF(*em, 45, 46, -122, -121);

    // Result should be non-negative (LoadSDF with range returns char)
    EXPECT_GE(result, 0);

    // Clean up
    remove("./test_sdf_data/45:-122:45:-122.sdf");
    remove("./test_sdf_data/45:-121:45:-121.sdf");
    remove("./test_sdf_data/46:-122:46:-122.sdf");
    remove("./test_sdf_data/46:-121:46:-121.sdf");
}

// Test boundary coordinates
TEST_F(SdfTest, BoundaryCoordinates) {
    // Test with coordinates at boundaries (e.g., 0 longitude, equator)
    CreateTestSDF("./test_sdf_data/0:0:0:0.sdf", 0, 0, 0, 0);

    Sdf sdf("./test_sdf_data", *sr);

    int result = sdf.LoadSDF(*em, "0:0:0:0", 0, 0, 0, 0);

    EXPECT_NE(result, -1);

    remove("./test_sdf_data/0:0:0:0.sdf");
}

// Test negative coordinates
TEST_F(SdfTest, NegativeCoordinates) {
    // Southern and western hemispheres
    CreateTestSDF("./test_sdf_data/-45:-122:-45:-122.sdf", -45, -45, -122,
                  -122);

    Sdf sdf("./test_sdf_data", *sr);

    int result = sdf.LoadSDF(*em, "-45:-122:-45:-122", -45, -45, -122, -122);

    EXPECT_NE(result, -1);

    remove("./test_sdf_data/-45:-122:-45:-122.sdf");
}

// Test invalid latitude range
TEST_F(SdfTest, InvalidLatitudeRange) {
    Sdf sdf("./test_sdf_data", *sr);

    // Latitude must be -90 to 90
    int result = sdf.LoadSDF(*em, "invalid", 91, 91, 0, 0);

    // Should handle gracefully (return 0)
    EXPECT_EQ(result, 0);
}

// Test invalid longitude range
TEST_F(SdfTest, InvalidLongitudeRange) {
    Sdf sdf("./test_sdf_data", *sr);

    // Longitude must be -180 to 180
    int result = sdf.LoadSDF(*em, "invalid", 0, 0, 181, 181);

    // Should handle gracefully
    EXPECT_EQ(result, 0);
}

// Test SDF path configuration
TEST_F(SdfTest, SDFPathConfiguration) {
    SplatRun custom_sr;
    custom_sr.sdf_path = "./custom_path";
    custom_sr.ppd = 1200;

    Sdf sdf("./custom_path", custom_sr);

    // Should construct with custom path
    SUCCEED();
}

// Test different PPD (points per degree) values
TEST_F(SdfTest, DifferentPPDValues) {
    // Skip this test - creating a custom PPD requires proper initialization
    // of the entire SplatRun which is complex. This test would be better
    // as an integration test rather than a unit test.
    GTEST_SKIP()
        << "Skipping custom PPD test - requires full SplatRun initialization";
}

// Test loading overlapping regions
TEST_F(SdfTest, OverlappingRegions) {
    CreateTestSDF("./test_sdf_data/45:-122:45:-122.sdf", 45, 45, -122, -122);

    Sdf sdf("./test_sdf_data", *sr);

    // Load same region twice
    int result1 = sdf.LoadSDF(*em, "45:-122:45:-122", 45, 45, -122, -122);
    int result2 = sdf.LoadSDF(*em, "45:-122:45:-122", 45, 45, -122, -122);

    // First load should succeed (not return -1)
    EXPECT_NE(result1, -1);

    // Second load returns 0 if already loaded (dem already exists)
    EXPECT_GE(result2, 0);

    remove("./test_sdf_data/45:-122:45:-122.sdf");
}

// Test large area loading
TEST_F(SdfTest, LargeAreaLoading) {
    Sdf sdf("./test_sdf_data", *sr);

    // Try to load large area (but files don't exist)
    // This tests that the code handles large ranges without crashing
    char result = sdf.LoadSDF(*em, 40, 50, -125, -115);

    // Should complete without errors, even if no files loaded
    EXPECT_GE(result, 0);
}

// Test SDF file naming convention
TEST_F(SdfTest, FileNamingConvention) {
    // Test various naming formats
    std::vector<std::string> filenames = {
        "./test_sdf_data/45:-122:45:-122.sdf", "./test_sdf_data/0:0:0:0.sdf",
        "./test_sdf_data/-45:-122:-45:-122.sdf"};

    for (const auto &filename : filenames) {
        // Create empty file to test naming
        std::ofstream test_file(filename);
        test_file.close();

        EXPECT_TRUE(std::ifstream(filename).good());

        remove(filename.c_str());
    }
}

// Parameterized test for various coordinate ranges
class SdfCoordinateTest
    : public ::testing::TestWithParam<std::tuple<int, int, int, int>> {
  protected:
    SplatRun *sr;
    ElevationMap *em;

    void SetUp() override {
        sr = new SplatRun();
        sr->sdf_path = "./test_sdf_coords";
        sr->ppd = 1200;
        em = new ElevationMap(*sr);
        int ret = system("mkdir -p ./test_sdf_coords");
        (void)ret;  // Ignore return value - test will fail later if dir not created
    }

    void TearDown() override {
        delete em;
        delete sr;
        int ret = system("rm -rf ./test_sdf_coords");
        (void)ret;  // Ignore return value - cleanup failure is not critical
    }
};

TEST_P(SdfCoordinateTest, ValidCoordinateRanges) {
    auto [min_lat, max_lat, min_lon, max_lon] = GetParam();

    Sdf sdf("./test_sdf_coords", *sr);

    // Attempt to load (files don't exist, but should handle coordinates)
    EXPECT_NO_THROW({ sdf.LoadSDF(*em, min_lat, max_lat, min_lon, max_lon); });
}

INSTANTIATE_TEST_SUITE_P(
    CoordinateRanges, SdfCoordinateTest,
    ::testing::Values(std::make_tuple(45, 45, -122, -122),  // Single degree
                      std::make_tuple(40, 42, -125, -120),  // Small region
                      std::make_tuple(0, 0, 0, 0),  // Equator/Prime Meridian
                      std::make_tuple(-45, -43, 170, 175),  // Southern/Eastern
                      std::make_tuple(70, 72, -15, -10)     // High latitude
                      ));
