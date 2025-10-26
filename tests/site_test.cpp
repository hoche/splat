/**
 * @file site_test.cpp
 *
 * Unit tests for the Site class
 */

#include "../src/site.h"
#include <cmath>
#include <fstream>
#include <gtest/gtest.h>

class SiteTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Create a temporary QTH file for testing
        std::ofstream qth_file("test_site.qth");
        qth_file << "Test Site\n";
        qth_file << "45.5231\n";    // latitude (Portland, OR area)
        qth_file << "-122.6765\n";  // longitude
        qth_file << "50.0\n";       // altitude in feet
        qth_file.close();

        // Create another site for distance/azimuth tests
        std::ofstream qth_file2("test_site2.qth");
        qth_file2 << "Test Site 2\n";
        qth_file2 << "47.6062\n";    // latitude (Seattle area)
        qth_file2 << "-122.3321\n";  // longitude
        qth_file2 << "100.0\n";
        qth_file2.close();
    }

    void TearDown() override {
        remove("test_site.qth");
        remove("test_site2.qth");
    }
};

// Test default constructor
TEST_F(SiteTest, DefaultConstructor) {
    Site site;

    // Default constructor doesn't initialize members - this is just a smoke test
    // Verify the object can be constructed without crashing
    SUCCEED();
}

// Test loading a QTH file
TEST_F(SiteTest, LoadQTHFile) {
    Site site;
    site.LoadQTH("test_site.qth");

    EXPECT_EQ(site.name, "Test Site");
    EXPECT_NEAR(site.lat, 45.5231, 0.0001);
    // Longitude is stored as 0-360 (west longitude becomes positive)
    // -122.6765 becomes 237.3235 (360 - 122.6765)
    EXPECT_NEAR(site.lon, 237.3235, 0.0001);
    EXPECT_NEAR(site.alt, 50.0, 0.1);
    EXPECT_EQ(site.filename, "test_site.qth");
}

// Test constructor with filename
TEST_F(SiteTest, ConstructorWithFilename) {
    Site site("test_site.qth");

    EXPECT_EQ(site.name, "Test Site");
    EXPECT_NEAR(site.lat, 45.5231, 0.0001);
    // Longitude is stored as 0-360 (west longitude becomes positive)
    EXPECT_NEAR(site.lon, 237.3235, 0.0001);
    EXPECT_EQ(site.filename, "test_site.qth");
}

// Test distance calculation between two sites
TEST_F(SiteTest, DistanceCalculation) {
    Site site1("test_site.qth");
    Site site2("test_site2.qth");

    double distance = site1.Distance(site2);

    // Portland to Seattle is approximately 145 miles
    EXPECT_GT(distance, 140.0);
    EXPECT_LT(distance, 150.0);
}

// Test distance to same site is zero
TEST_F(SiteTest, DistanceToSelf) {
    Site site1("test_site.qth");

    double distance = site1.Distance(site1);

    EXPECT_NEAR(distance, 0.0, 0.001);
}

// Test azimuth calculation
TEST_F(SiteTest, AzimuthCalculation) {
    Site site1("test_site.qth");   // Portland area
    Site site2("test_site2.qth");  // Seattle area

    double azimuth = site1.Azimuth(site2);

    // Portland to Seattle should be roughly north (0 degrees) with slight east
    // Expected azimuth is around 350-360 degrees or 0-10 degrees
    EXPECT_TRUE((azimuth >= 350.0 && azimuth <= 360.0) ||
                (azimuth >= 0.0 && azimuth <= 10.0));
}

// Test azimuth is in valid range
TEST_F(SiteTest, AzimuthRange) {
    Site site1("test_site.qth");
    Site site2("test_site2.qth");

    double azimuth = site1.Azimuth(site2);

    EXPECT_GE(azimuth, 0.0);
    EXPECT_LT(azimuth, 360.0);
}

// Test reciprocal azimuth (reverse direction should differ by ~180 degrees)
TEST_F(SiteTest, ReciprocalAzimuth) {
    Site site1("test_site.qth");
    Site site2("test_site2.qth");

    double azimuth_1_to_2 = site1.Azimuth(site2);
    double azimuth_2_to_1 = site2.Azimuth(site1);

    // Reciprocal azimuths should differ by approximately 180 degrees
    double difference = std::abs(azimuth_1_to_2 - azimuth_2_to_1);

    // Account for wrap-around at 0/360
    if (difference > 180.0) {
        difference = 360.0 - difference;
    }

    EXPECT_NEAR(difference, 180.0, 5.0);  // Allow 5 degree tolerance
}

// Test loading non-existent QTH file
TEST_F(SiteTest, LoadNonExistentFile) {
    Site site;

    // Should not throw, just leave site with default values
    EXPECT_NO_THROW(site.LoadQTH("nonexistent.qth"));

    // Site should still have default/empty values
    EXPECT_EQ(site.name, "");
}

// Test with international coordinates (different hemisphere)
TEST_F(SiteTest, InternationalCoordinates) {
    // Create a site in the southern hemisphere
    std::ofstream qth_file("test_south.qth");
    qth_file << "Southern Site\n";
    qth_file << "-33.8688\n";  // Sydney, Australia latitude
    qth_file << "151.2093\n";  // longitude
    qth_file << "20.0\n";
    qth_file.close();

    Site site("test_south.qth");

    EXPECT_NEAR(site.lat, -33.8688, 0.0001);
    EXPECT_NEAR(site.lon, 151.2093, 0.0001);

    remove("test_south.qth");
}

// Test distance across hemispheres
TEST_F(SiteTest, TransHemisphereDistance) {
    // Create two sites on opposite sides of equator
    std::ofstream north("test_north.qth");
    north << "North\n40.0\n-100.0\n100.0\n";
    north.close();

    std::ofstream south("test_south.qth");
    south << "South\n-40.0\n-100.0\n100.0\n";
    south.close();

    Site site_north("test_north.qth");
    Site site_south("test_south.qth");

    double distance = site_north.Distance(site_south);

    // Distance should be positive and substantial (thousands of miles)
    EXPECT_GT(distance, 5000.0);

    remove("test_north.qth");
    remove("test_south.qth");
}

// Test site with very high altitude
TEST_F(SiteTest, HighAltitude) {
    std::ofstream qth_file("test_mountain.qth");
    qth_file << "Mountain Peak\n";
    qth_file << "45.0\n";
    qth_file << "-120.0\n";
    qth_file << "14000.0\n";  // High altitude in feet
    qth_file.close();

    Site site("test_mountain.qth");

    EXPECT_NEAR(site.alt, 14000.0, 1.0);

    remove("test_mountain.qth");
}

// Test site name with spaces
TEST_F(SiteTest, SiteNameWithSpaces) {
    std::ofstream qth_file("test_spaces.qth");
    qth_file << "Site With Multiple Spaces\n";
    qth_file << "45.0\n-120.0\n100.0\n";
    qth_file.close();

    Site site("test_spaces.qth");

    EXPECT_EQ(site.name, "Site With Multiple Spaces");

    remove("test_spaces.qth");
}

// Parameterized test for various coordinate pairs
class SiteDistanceTest
    : public ::testing::TestWithParam<
          std::tuple<double, double, double, double, double, double>> { };

TEST_P(SiteDistanceTest, VerifyDistanceSymmetry) {
    auto [lat1, lon1, lat2, lon2, min_dist, max_dist] = GetParam();

    std::ofstream qth1("temp1.qth");
    qth1 << "Site1\n" << lat1 << "\n" << lon1 << "\n100.0\n";
    qth1.close();

    std::ofstream qth2("temp2.qth");
    qth2 << "Site2\n" << lat2 << "\n" << lon2 << "\n100.0\n";
    qth2.close();

    Site site1("temp1.qth");
    Site site2("temp2.qth");

    double distance_1_to_2 = site1.Distance(site2);
    double distance_2_to_1 = site2.Distance(site1);

    // Distance should be symmetric
    EXPECT_NEAR(distance_1_to_2, distance_2_to_1, 0.001);

    // Distance should be in expected range
    EXPECT_GE(distance_1_to_2, min_dist);
    EXPECT_LE(distance_1_to_2, max_dist);

    remove("temp1.qth");
    remove("temp2.qth");
}

INSTANTIATE_TEST_SUITE_P(
    VariousLocations, SiteDistanceTest,
    ::testing::Values(
        // lat1, lon1, lat2, lon2, min_distance, max_distance
        std::make_tuple(40.7128, -74.0060, 34.0522, -118.2437, 2400.0,
                        2500.0),  // NYC to LA
        std::make_tuple(51.5074, -0.1278, 48.8566, 2.3522, 200.0,
                        250.0),  // London to Paris
        std::make_tuple(0.0, 0.0, 0.0, 1.0, 60.0,
                        70.0),  // 1 degree longitude at equator
        std::make_tuple(45.0, 0.0, 46.0, 0.0, 65.0, 75.0)  // 1 degree latitude
        ));
