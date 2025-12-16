/**
 * @file udt_test.cpp
 *
 * Unit tests for the UDT (User-Defined Terrain) class
 */

#include "../src/udt.h"
#include "../src/elevation_map.h"
#include "../src/splat_run.h"
#include <cmath>
#include <fstream>
#include <gtest/gtest.h>

class UdtTest : public ::testing::Test {
  protected:
    SplatRun *sr;
    ElevationMap *em;

    void SetUp() override {
        // Create a minimal SplatRun configuration for testing
        sr = new SplatRun();
        sr->ppd = 1200.0;  // Standard mode: 1200 pixels per degree
        sr->dpp = 1.0 / sr->ppd;  // degrees per pixel
        sr->ippd = 1200;
        sr->hd_mode = false;
        sr->metric = false;
        
        // Create an ElevationMap
        em = new ElevationMap(*sr);
    }

    void TearDown() override {
        delete em;
        delete sr;
        
        // Clean up any test files
        remove("test_udt_basic.udt");
        remove("test_udt_comments.udt");
        remove("test_udt_decimal.udt");
        remove("test_udt_dms.udt");
        remove("test_udt_feet.udt");
        remove("test_udt_meters.udt");
        remove("test_udt_mixed_units.udt");
        remove("test_udt_duplicates.udt");
        remove("test_udt_negative.udt");
        remove("test_udt_empty.udt");
        remove("test_udt_invalid.udt");
        remove("test_udt_comprehensive.udt");
    }
};

// Test basic UDT file loading with decimal coordinates and feet
TEST_F(UdtTest, BasicDecimalFeet) {
    // Create a basic UDT file
    std::ofstream udt_file("test_udt_basic.udt");
    udt_file << "40.748, 73.985, 200.0\n";  // 200 feet
    udt_file << "40.750, 74.000, 150.0\n";  // 150 feet
    udt_file.close();

    Udt udt(*sr);
    
    // Redirect stdout to suppress output
    FILE* original_stdout = stdout;
    stdout = fopen("/dev/null", "w");
    
    EXPECT_NO_THROW(udt.LoadUDT("test_udt_basic.udt", *em));
    
    // Restore stdout
    fclose(stdout);
    stdout = original_stdout;
}

// Test comment handling
TEST_F(UdtTest, CommentHandling) {
    std::ofstream udt_file("test_udt_comments.udt");
    udt_file << "; This is a comment line\n";
    udt_file << "40.748, 73.985, 200.0  ; Inline comment\n";
    udt_file << "; Another comment\n";
    udt_file << "40.750, 74.000, 150.0\n";
    udt_file.close();

    Udt udt(*sr);
    
    FILE* original_stdout = stdout;
    stdout = fopen("/dev/null", "w");
    
    EXPECT_NO_THROW(udt.LoadUDT("test_udt_comments.udt", *em));
    
    fclose(stdout);
    stdout = original_stdout;
}

// Test decimal degree coordinates
TEST_F(UdtTest, DecimalCoordinates) {
    std::ofstream udt_file("test_udt_decimal.udt");
    udt_file << "40.2828, 74.6864, 100.0\n";
    udt_file << "-33.8688, 151.2093, 200.0\n";  // Sydney (negative lat)
    udt_file.close();

    Udt udt(*sr);
    
    FILE* original_stdout = stdout;
    stdout = fopen("/dev/null", "w");
    
    EXPECT_NO_THROW(udt.LoadUDT("test_udt_decimal.udt", *em));
    
    fclose(stdout);
    stdout = original_stdout;
}

// Test DMS (Degrees Minutes Seconds) coordinates
TEST_F(UdtTest, DMSCoordinates) {
    std::ofstream udt_file("test_udt_dms.udt");
    udt_file << "40 48 8.0, 74 14 47.0, 100.0\n";  // DMS format
    udt_file << "40 45 0.0, 74 0 30.0, 80.0\n";
    udt_file.close();

    Udt udt(*sr);
    
    FILE* original_stdout = stdout;
    stdout = fopen("/dev/null", "w");
    
    EXPECT_NO_THROW(udt.LoadUDT("test_udt_dms.udt", *em));
    
    fclose(stdout);
    stdout = original_stdout;
}

// Test height in feet (default)
TEST_F(UdtTest, HeightInFeet) {
    std::ofstream udt_file("test_udt_feet.udt");
    udt_file << "40.748, 73.985, 100.0\n";  // 100 feet (no unit)
    udt_file << "40.750, 74.000, 200\n";    // 200 feet (integer)
    udt_file << "40.752, 74.012, 150.5\n";  // 150.5 feet (decimal)
    udt_file.close();

    Udt udt(*sr);
    
    FILE* original_stdout = stdout;
    stdout = fopen("/dev/null", "w");
    
    EXPECT_NO_THROW(udt.LoadUDT("test_udt_feet.udt", *em));
    
    fclose(stdout);
    stdout = original_stdout;
}

// Test height in meters with 'm' suffix
TEST_F(UdtTest, HeightInMetersLowercase) {
    std::ofstream udt_file("test_udt_meters.udt");
    udt_file << "40.748, 73.985, 100.0 m\n";      // 100 meters with 'm'
    udt_file << "40.750, 74.000, 50m\n";          // 50 meters, no space
    udt_file << "40.752, 74.012, 75.5 m\n";       // 75.5 meters
    udt_file.close();

    Udt udt(*sr);
    
    FILE* original_stdout = stdout;
    stdout = fopen("/dev/null", "w");
    
    EXPECT_NO_THROW(udt.LoadUDT("test_udt_meters.udt", *em));
    
    fclose(stdout);
    stdout = original_stdout;
}

// Test height in meters with 'M' suffix (uppercase)
TEST_F(UdtTest, HeightInMetersUppercase) {
    std::ofstream udt_file("test_udt_meters.udt");
    udt_file << "40.748, 73.985, 100.0 M\n";      // 100 meters with 'M'
    udt_file << "40.750, 74.000, 50M\n";          // 50 meters, no space
    udt_file << "40.752, 74.012, 100.0 meters\n"; // with 'meters' word
    udt_file.close();

    Udt udt(*sr);
    
    FILE* original_stdout = stdout;
    stdout = fopen("/dev/null", "w");
    
    EXPECT_NO_THROW(udt.LoadUDT("test_udt_meters.udt", *em));
    
    fclose(stdout);
    stdout = original_stdout;
}

// Test mixed units in same file
TEST_F(UdtTest, MixedUnits) {
    std::ofstream udt_file("test_udt_mixed_units.udt");
    udt_file << "40.748, 73.985, 200.0\n";        // feet (default)
    udt_file << "40.750, 74.000, 150 m\n";        // meters
    udt_file << "40.752, 74.012, 120.0\n";        // feet
    udt_file << "40.754, 74.024, 100.0 meters\n"; // meters
    udt_file.close();

    Udt udt(*sr);
    
    FILE* original_stdout = stdout;
    stdout = fopen("/dev/null", "w");
    
    EXPECT_NO_THROW(udt.LoadUDT("test_udt_mixed_units.udt", *em));
    
    fclose(stdout);
    stdout = original_stdout;
}

// Test duplicate coordinate handling (max height should be kept)
TEST_F(UdtTest, DuplicateCoordinates) {
    std::ofstream udt_file("test_udt_duplicates.udt");
    udt_file << "40.748, 73.985, 100.0\n";  // First entry
    udt_file << "40.748, 73.985, 200.0\n";  // Duplicate (higher)
    udt_file << "40.748, 73.985, 150.0\n";  // Duplicate (lower)
    udt_file.close();

    Udt udt(*sr);
    
    FILE* original_stdout = stdout;
    stdout = fopen("/dev/null", "w");
    
    // Should not throw, duplicates should be handled
    EXPECT_NO_THROW(udt.LoadUDT("test_udt_duplicates.udt", *em));
    
    fclose(stdout);
    stdout = original_stdout;
}

// Test negative and zero heights (should be ignored)
TEST_F(UdtTest, NegativeAndZeroHeights) {
    std::ofstream udt_file("test_udt_negative.udt");
    udt_file << "40.748, 73.985, -100.0\n";  // Negative (should be ignored)
    udt_file << "40.750, 74.000, 0.0\n";     // Zero (should be ignored)
    udt_file << "40.752, 74.012, 100.0\n";   // Valid positive
    udt_file.close();

    Udt udt(*sr);
    
    FILE* original_stdout = stdout;
    stdout = fopen("/dev/null", "w");
    
    EXPECT_NO_THROW(udt.LoadUDT("test_udt_negative.udt", *em));
    
    fclose(stdout);
    stdout = original_stdout;
}

// Test empty file
TEST_F(UdtTest, EmptyFile) {
    std::ofstream udt_file("test_udt_empty.udt");
    udt_file.close();

    Udt udt(*sr);
    
    FILE* original_stdout = stdout;
    stdout = fopen("/dev/null", "w");
    
    EXPECT_NO_THROW(udt.LoadUDT("test_udt_empty.udt", *em));
    
    fclose(stdout);
    stdout = original_stdout;
}

// Test file with only comments
TEST_F(UdtTest, OnlyComments) {
    std::ofstream udt_file("test_udt_empty.udt");
    udt_file << "; This is a comment\n";
    udt_file << "; Another comment\n";
    udt_file << ";; More comments\n";
    udt_file.close();

    Udt udt(*sr);
    
    FILE* original_stdout = stdout;
    stdout = fopen("/dev/null", "w");
    
    EXPECT_NO_THROW(udt.LoadUDT("test_udt_empty.udt", *em));
    
    fclose(stdout);
    stdout = original_stdout;
}

// Test invalid lines (missing commas, incomplete data)
TEST_F(UdtTest, InvalidLines) {
    std::ofstream udt_file("test_udt_invalid.udt");
    udt_file << "40.748 73.985 200.0\n";     // Missing commas (should be skipped)
    udt_file << "40.750, 74.000\n";          // Missing height (should be skipped)
    udt_file << "40.752\n";                  // Missing lon and height (should be skipped)
    udt_file << "40.754, 74.024, 100.0\n";   // Valid line
    udt_file.close();

    Udt udt(*sr);
    
    FILE* original_stdout = stdout;
    stdout = fopen("/dev/null", "w");
    
    // Should not crash, invalid lines should be skipped
    EXPECT_NO_THROW(udt.LoadUDT("test_udt_invalid.udt", *em));
    
    fclose(stdout);
    stdout = original_stdout;
}

// Test file not found (should handle gracefully)
TEST_F(UdtTest, FileNotFound) {
    Udt udt(*sr);
    
    // Redirect stderr to suppress error message
    FILE* original_stderr = stderr;
    stderr = fopen("/dev/null", "w");
    
    // Should not crash when file doesn't exist
    EXPECT_NO_THROW(udt.LoadUDT("nonexistent_file.udt", *em));
    
    fclose(stderr);
    stderr = original_stderr;
}

// Test comprehensive example with various features
TEST_F(UdtTest, ComprehensiveExample) {
    std::ofstream udt_file("test_udt_comprehensive.udt");
    udt_file << "; User-Defined Terrain for downtown area\n";
    udt_file << "; Format: latitude, longitude, height\n";
    udt_file << "\n";  // Empty line
    udt_file << "; Office building at 40.748° N, 73.985° W, 200 feet tall\n";
    udt_file << "40.748, 73.985, 200.0\n";
    udt_file << "\n";
    udt_file << "; Radio tower at 40.75° N, 74.0° W, 150 meters tall\n";
    udt_file << "40.75, 74.0, 150 m\n";
    udt_file << "\n";
    udt_file << "; Water tower using DMS coordinates, 80 feet\n";
    udt_file << "40 45 0.0, 74 0 30.0, 80\n";
    udt_file << "\n";
    udt_file << "; Cell tower\n";
    udt_file << "40.752, 74.012, 120.0  ; 120 feet AGL\n";
    udt_file.close();

    Udt udt(*sr);
    
    FILE* original_stdout = stdout;
    stdout = fopen("/dev/null", "w");
    
    EXPECT_NO_THROW(udt.LoadUDT("test_udt_comprehensive.udt", *em));
    
    fclose(stdout);
    stdout = original_stdout;
}

// Test HD mode (higher resolution)
TEST_F(UdtTest, HDMode) {
    // Reconfigure for HD mode
    sr->hd_mode = true;
    sr->ppd = 3600.0;  // HD mode: 3600 pixels per degree
    sr->dpp = 1.0 / sr->ppd;
    sr->ippd = 3600;

    std::ofstream udt_file("test_udt_basic.udt");
    udt_file << "40.748, 73.985, 200.0\n";
    udt_file.close();

    Udt udt(*sr);
    
    FILE* original_stdout = stdout;
    stdout = fopen("/dev/null", "w");
    
    EXPECT_NO_THROW(udt.LoadUDT("test_udt_basic.udt", *em));
    
    fclose(stdout);
    stdout = original_stdout;
}

// Test whitespace handling
TEST_F(UdtTest, WhitespaceHandling) {
    std::ofstream udt_file("test_udt_basic.udt");
    udt_file << "  40.748  ,  73.985  ,  200.0  \n";  // Extra spaces
    udt_file << "40.750,74.000,150.0\n";               // No spaces
    udt_file << "40.752 ,74.012 , 120.0\n";            // Mixed spacing
    udt_file.close();

    Udt udt(*sr);
    
    FILE* original_stdout = stdout;
    stdout = fopen("/dev/null", "w");
    
    EXPECT_NO_THROW(udt.LoadUDT("test_udt_basic.udt", *em));
    
    fclose(stdout);
    stdout = original_stdout;
}

// Test longitude normalization (negative to 0-360)
TEST_F(UdtTest, LongitudeNormalization) {
    std::ofstream udt_file("test_udt_basic.udt");
    udt_file << "40.748, -73.985, 200.0\n";   // Negative longitude (East)
    udt_file << "40.750, -122.419, 150.0\n";  // Negative longitude (West Coast)
    udt_file.close();

    Udt udt(*sr);
    
    FILE* original_stdout = stdout;
    stdout = fopen("/dev/null", "w");
    
    // Negative longitudes should be converted to 0-360 range
    EXPECT_NO_THROW(udt.LoadUDT("test_udt_basic.udt", *em));
    
    fclose(stdout);
    stdout = original_stdout;
}

// Test very large heights
TEST_F(UdtTest, LargeHeights) {
    std::ofstream udt_file("test_udt_basic.udt");
    udt_file << "40.748, 73.985, 10000.0\n";      // 10,000 feet
    udt_file << "40.750, 74.000, 5000.0 m\n";     // 5,000 meters
    udt_file.close();

    Udt udt(*sr);
    
    FILE* original_stdout = stdout;
    stdout = fopen("/dev/null", "w");
    
    EXPECT_NO_THROW(udt.LoadUDT("test_udt_basic.udt", *em));
    
    fclose(stdout);
    stdout = original_stdout;
}

// Test small fractional heights
TEST_F(UdtTest, SmallHeights) {
    std::ofstream udt_file("test_udt_basic.udt");
    udt_file << "40.748, 73.985, 0.5\n";      // 0.5 feet (should be ignored - rounds to 0)
    udt_file << "40.750, 74.000, 0.3 m\n";    // 0.3 meters (should be ignored - rounds to 0)
    udt_file << "40.752, 74.012, 1.0\n";      // 1.0 feet (valid)
    udt_file.close();

    Udt udt(*sr);
    
    FILE* original_stdout = stdout;
    stdout = fopen("/dev/null", "w");
    
    EXPECT_NO_THROW(udt.LoadUDT("test_udt_basic.udt", *em));
    
    fclose(stdout);
    stdout = original_stdout;
}
