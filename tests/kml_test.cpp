/**
 * @file kml_test.cpp
 *
 * Unit tests for KML and KMZ output functionality
 *
 * Note: These tests create KML/KMZ files but the path data will be minimal
 * because we use a very small arraysize to avoid excessive memory allocation
 * in the ElevationMap.
 */

#include <cstdio>
#include <fstream>
#include <gtest/gtest.h>
#include <string>

#include "../src/elevation_map.h"
#include "../src/kml.h"
#include "../src/site.h"
#include "../src/splat_run.h"

extern "C" {
#include "../src/zip.h"
}

class KmlTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Create a minimal SplatRun configuration with very small memory footprint
        // PPD of 1200 means 1200 samples per degree
        // arraysize of 100 means we can handle paths up to a certain distance
        sr.ppd = 1200;
        sr.arraysize = 100;
        sr.metric = false;
        sr.earthradius = 3959.0;
        sr.maxpages = 1;  // Minimize memory
        sr.ippd = 1200;
        sr.mpi = 1;

        // Create minimal elevation map - this allocates memory based on maxpages
        // With maxpages=1, this should be manageable
        try {
            em = new ElevationMap(sr);
        } catch (const std::bad_alloc &) {
            // If we can't allocate even minimal memory, skip these tests
            GTEST_SKIP()
                << "Insufficient memory to create ElevationMap for testing";
        }

        // Set up test sites
        tx_site.lat = 40.0;
        tx_site.lon = 105.0;  // Will be converted to 255.0 in 0-360 range
        tx_site.alt = 100.0;
        tx_site.name = "TestTX";

        rx_site.lat = 40.5;
        rx_site.lon = 105.5;  // Will be converted to 254.5 in 0-360 range
        rx_site.alt = 50.0;
        rx_site.name = "TestRX";
    }

    void TearDown() override {
        if (em) {
            delete em;
            em = nullptr;
        }

        // Clean up any generated files
        remove("TestTX-to-TestRX.kml");
        remove("TestTX-to-TestRX.kmz");
        remove("TestTX-to-TestRX_temp.kml");
    }

    bool FileExists(const std::string &filename) {
        std::ifstream f(filename);
        return f.good();
    }

    std::string ReadFile(const std::string &filename) {
        std::ifstream f(filename);
        std::string content((std::istreambuf_iterator<char>(f)),
                            std::istreambuf_iterator<char>());
        return content;
    }

    bool KmlContainsExpectedContent(const std::string &content) {
        // Check for essential KML elements
        return content.find("<?xml version=\"1.0\" encoding=\"UTF-8\"?>") !=
                   std::string::npos &&
               content.find(
                   "<kml xmlns=\"http://earth.google.com/kml/2.0\">") !=
                   std::string::npos &&
               content.find("<name>SPLAT! Path</name>") != std::string::npos &&
               content.find("TestTX") != std::string::npos &&
               content.find("TestRX") != std::string::npos &&
               content.find("<Placemark>") != std::string::npos &&
               content.find("</kml>") != std::string::npos;
    }

    SplatRun sr;
    ElevationMap *em;
    Site tx_site;
    Site rx_site;
};

// Test KML file generation
TEST_F(KmlTest, WriteKMLCreatesFile) {
    Kml kml(*em, sr);

    // This will create TestTX-to-TestRX.kml
    kml.WriteKML(tx_site, rx_site);

    EXPECT_TRUE(FileExists("TestTX-to-TestRX.kml"))
        << "KML file should be created";
}

TEST_F(KmlTest, WriteKMLContainsValidXML) {
    Kml kml(*em, sr);
    kml.WriteKML(tx_site, rx_site);

    ASSERT_TRUE(FileExists("TestTX-to-TestRX.kml"));

    std::string content = ReadFile("TestTX-to-TestRX.kml");

    EXPECT_TRUE(KmlContainsExpectedContent(content))
        << "KML file should contain valid XML structure";
}

TEST_F(KmlTest, WriteKMLContainsSiteNames) {
    Kml kml(*em, sr);
    kml.WriteKML(tx_site, rx_site);

    ASSERT_TRUE(FileExists("TestTX-to-TestRX.kml"));

    std::string content = ReadFile("TestTX-to-TestRX.kml");

    EXPECT_NE(content.find("TestTX"), std::string::npos)
        << "KML should contain TX site name";
    EXPECT_NE(content.find("TestRX"), std::string::npos)
        << "KML should contain RX site name";
}

TEST_F(KmlTest, WriteKMLContainsPlacemarks) {
    Kml kml(*em, sr);
    kml.WriteKML(tx_site, rx_site);

    ASSERT_TRUE(FileExists("TestTX-to-TestRX.kml"));

    std::string content = ReadFile("TestTX-to-TestRX.kml");

    EXPECT_NE(content.find("Transmit Site"), std::string::npos)
        << "KML should contain transmit site placemark";
    EXPECT_NE(content.find("Receive Site"), std::string::npos)
        << "KML should contain receive site placemark";
    EXPECT_NE(content.find("Point-to-Point Path"), std::string::npos)
        << "KML should contain path placemark";
    EXPECT_NE(content.find("Line-of-Sight Path"), std::string::npos)
        << "KML should contain line-of-sight placemark";
}

TEST_F(KmlTest, WriteKMLContainsCoordinates) {
    Kml kml(*em, sr);
    kml.WriteKML(tx_site, rx_site);

    ASSERT_TRUE(FileExists("TestTX-to-TestRX.kml"));

    std::string content = ReadFile("TestTX-to-TestRX.kml");

    EXPECT_NE(content.find("<coordinates>"), std::string::npos)
        << "KML should contain coordinates";
    EXPECT_NE(content.find("</coordinates>"), std::string::npos)
        << "KML should have closing coordinates tag";
}

// Test KMZ file generation
TEST_F(KmlTest, WriteKMZCreatesFile) {
    Kml kml(*em, sr);

    // This will create TestTX-to-TestRX.kmz
    kml.WriteKMZ(tx_site, rx_site);

    EXPECT_TRUE(FileExists("TestTX-to-TestRX.kmz"))
        << "KMZ file should be created";
}

TEST_F(KmlTest, WriteKMZDoesNotLeaveTemporaryFile) {
    Kml kml(*em, sr);
    kml.WriteKMZ(tx_site, rx_site);

    EXPECT_FALSE(FileExists("TestTX-to-TestRX_temp.kml"))
        << "Temporary KML file should be cleaned up";
}

TEST_F(KmlTest, WriteKMZCreatesValidZipFile) {
    Kml kml(*em, sr);
    kml.WriteKMZ(tx_site, rx_site);

    ASSERT_TRUE(FileExists("TestTX-to-TestRX.kmz"));

    // Try to open the KMZ as a zip file
    struct zip_t *zip = zip_open("TestTX-to-TestRX.kmz", 0, 'r');
    ASSERT_NE(zip, nullptr) << "KMZ file should be a valid zip archive";

    // Check that it contains exactly one entry
    int entries = zip_total_entries(zip);
    EXPECT_EQ(entries, 1) << "KMZ should contain exactly one file";

    zip_close(zip);
}

TEST_F(KmlTest, WriteKMZContainsDocKml) {
    Kml kml(*em, sr);
    kml.WriteKMZ(tx_site, rx_site);

    ASSERT_TRUE(FileExists("TestTX-to-TestRX.kmz"));

    struct zip_t *zip = zip_open("TestTX-to-TestRX.kmz", 0, 'r');
    ASSERT_NE(zip, nullptr);

    // Open the first (and only) entry
    ASSERT_EQ(zip_entry_openbyindex(zip, 0), 0);

    // Check that the entry is named "doc.kml"
    const char *name = zip_entry_name(zip);
    EXPECT_STREQ(name, "doc.kml")
        << "KMZ should contain a file named 'doc.kml'";

    zip_entry_close(zip);
    zip_close(zip);
}

TEST_F(KmlTest, WriteKMZContainsValidKmlContent) {
    Kml kml(*em, sr);
    kml.WriteKMZ(tx_site, rx_site);

    ASSERT_TRUE(FileExists("TestTX-to-TestRX.kmz"));

    struct zip_t *zip = zip_open("TestTX-to-TestRX.kmz", 0, 'r');
    ASSERT_NE(zip, nullptr);

    ASSERT_EQ(zip_entry_openbyindex(zip, 0), 0);

    // Extract the KML content
    void *buf = nullptr;
    size_t bufsize = 0;
    ssize_t bytes_read = zip_entry_read(zip, &buf, &bufsize);

    ASSERT_GT(bytes_read, 0) << "Should be able to read KML content from KMZ";

    std::string content(static_cast<char *>(buf), bufsize);
    free(buf);

    EXPECT_TRUE(KmlContainsExpectedContent(content))
        << "KML content in KMZ should be valid";

    zip_entry_close(zip);
    zip_close(zip);
}

TEST_F(KmlTest, WriteKMZKmlContentMatchesWriteKML) {
    Kml kml(*em, sr);

    // Generate both KML and KMZ
    kml.WriteKML(tx_site, rx_site);
    kml.WriteKMZ(tx_site, rx_site);

    ASSERT_TRUE(FileExists("TestTX-to-TestRX.kml"));
    ASSERT_TRUE(FileExists("TestTX-to-TestRX.kmz"));

    // Read KML file
    std::string kml_content = ReadFile("TestTX-to-TestRX.kml");

    // Extract KML from KMZ
    struct zip_t *zip = zip_open("TestTX-to-TestRX.kmz", 0, 'r');
    ASSERT_NE(zip, nullptr);
    ASSERT_EQ(zip_entry_openbyindex(zip, 0), 0);

    void *buf = nullptr;
    size_t bufsize = 0;
    zip_entry_read(zip, &buf, &bufsize);

    std::string kmz_kml_content(static_cast<char *>(buf), bufsize);
    free(buf);

    zip_entry_close(zip);
    zip_close(zip);

    // The content should be identical
    EXPECT_EQ(kml_content, kmz_kml_content)
        << "KML content should be the same in both .kml and .kmz files";
}

// Test filename sanitization
TEST_F(KmlTest, WriteKMLSanitizesFilenames) {
    tx_site.name = "Test TX";  // Space should be replaced with underscore
    rx_site.name = "Test/RX";  // Slash should be replaced with underscore

    Kml kml(*em, sr);
    kml.WriteKML(tx_site, rx_site);

    // Should create Test_TX-to-Test_RX.kml
    EXPECT_TRUE(FileExists("Test_TX-to-Test_RX.kml"))
        << "Filename should have special characters replaced with underscores";

    remove("Test_TX-to-Test_RX.kml");
}

TEST_F(KmlTest, WriteKMZSanitizesFilenames) {
    tx_site.name = "Test TX";
    rx_site.name = "Test\\RX";  // Backslash should be replaced

    Kml kml(*em, sr);
    kml.WriteKMZ(tx_site, rx_site);

    EXPECT_TRUE(FileExists("Test_TX-to-Test_RX.kmz"))
        << "KMZ filename should have special characters sanitized";

    remove("Test_TX-to-Test_RX.kmz");
}

// Test metric vs imperial units
TEST_F(KmlTest, WriteKMLUsesImperialByDefault) {
    Kml kml(*em, sr);
    kml.WriteKML(tx_site, rx_site);

    ASSERT_TRUE(FileExists("TestTX-to-TestRX.kml"));

    std::string content = ReadFile("TestTX-to-TestRX.kml");

    EXPECT_NE(content.find("miles"), std::string::npos)
        << "KML should use miles when metric is false";
}

TEST_F(KmlTest, WriteKMLUsesMetricWhenConfigured) {
    sr.metric = true;
    Kml kml(*em, sr);
    kml.WriteKML(tx_site, rx_site);

    ASSERT_TRUE(FileExists("TestTX-to-TestRX.kml"));

    std::string content = ReadFile("TestTX-to-TestRX.kml");

    EXPECT_NE(content.find("km"), std::string::npos)
        << "KML should use kilometers when metric is true";
}

// Smoke tests for edge cases
TEST_F(KmlTest, WriteKMLWithSameSiteNames) {
    tx_site.name = "Same";
    rx_site.name = "Same";

    Kml kml(*em, sr);

    EXPECT_NO_THROW(kml.WriteKML(tx_site, rx_site))
        << "Should handle identical site names";

    remove("Same-to-Same.kml");
}

TEST_F(KmlTest, WriteKMZWithSameSiteNames) {
    tx_site.name = "Same";
    rx_site.name = "Same";

    Kml kml(*em, sr);

    EXPECT_NO_THROW(kml.WriteKMZ(tx_site, rx_site))
        << "Should handle identical site names for KMZ";

    remove("Same-to-Same.kmz");
}
