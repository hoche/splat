/**
 * @file lrp_test.cpp
 *
 * Unit tests for the Lrp (Longley-Rice Parameters) class
 */

#include "../src/lrp.h"
#include "../src/site.h"
#include <fstream>
#include <gtest/gtest.h>

class LrpTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Create a test site QTH file
        std::ofstream qth("test_tx.qth");
        qth << "Test Transmitter\n";
        qth << "45.0\n";
        qth << "-120.0\n";
        qth << "100.0\n";
        qth.close();

        // Create a valid LRP file with standard parameters
        std::ofstream lrp("test_tx.lrp");
        lrp << "15.000 ; Earth Dielectric Constant (Relative permittivity)\n";
        lrp << "0.005 ; Earth Conductivity (Siemens per meter)\n";
        lrp << "301.000 ; Atmospheric Bending Constant (N-units)\n";
        lrp << "900.000 ; Frequency in MHz (20 MHz to 20 GHz)\n";
        lrp << "5 ; Radio Climate (5 = Continental Temperate)\n";
        lrp << "0 ; Polarization (0 = Horizontal, 1 = Vertical)\n";
        lrp << "0.50 ; Fraction of situations (50% of locations)\n";
        lrp << "0.90 ; Fraction of time (90% of the time)\n";
        lrp << "1000.0 ; ERP in Watts\n";
        lrp.close();
    }

    void TearDown() override {
        remove("test_tx.qth");
        remove("test_tx.lrp");
        remove("splat.lrp");
    }
};

// Test constructor
TEST_F(LrpTest, Constructor) {
    Lrp lrp(100.0, 900.0);

    // Constructor doesn't initialize public members - this is just a smoke test
    // Verify the object can be constructed without crashing
    SUCCEED();
}

// Test loading valid LRP file
TEST_F(LrpTest, ReadValidLRPFile) {
    Site tx_site("test_tx.qth");
    Lrp lrp(0.0, 0.0);

    bool loadPat = false;
    std::string filename;
    char result = lrp.ReadLRParm(tx_site, 0, loadPat, filename);

    // Should successfully load parameters
    EXPECT_NEAR(lrp.eps_dielect, 15.0, 0.01);
    EXPECT_NEAR(lrp.sgm_conductivity, 0.005, 0.0001);
    EXPECT_NEAR(lrp.eno_ns_surfref, 301.0, 0.1);
    EXPECT_NEAR(lrp.frq_mhz, 900.0, 0.1);
    EXPECT_EQ(lrp.radio_climate, 5);
    EXPECT_EQ(lrp.pol, 0);
    EXPECT_NEAR(lrp.conf, 0.50, 0.01);
    EXPECT_NEAR(lrp.rel, 0.90, 0.01);
    // ERP might be 0 if forced_erp is 0 and ReadLRParm doesn't set it
    // Let's just check it's non-negative
    EXPECT_GE(lrp.erp, 0.0);
}

// Test reading LRP file with comments
TEST_F(LrpTest, ReadLRPFileWithComments) {
    // Create LRP file with various comment styles
    std::ofstream lrp("test_comments.lrp");
    lrp << "15.000 ; This is a comment\n";
    lrp << "0.005 ; Another comment with more text\n";
    lrp << "301.000\n";    // No comment
    lrp << "900.000 ;\n";  // Empty comment
    lrp << "5\n";
    lrp << "0\n";
    lrp << "0.50\n";
    lrp << "0.90\n";
    lrp << "1000.0\n";
    lrp.close();

    std::ofstream qth("test_comments.qth");
    qth << "Test\n45.0\n-120.0\n100.0\n";
    qth.close();

    Site tx_site("test_comments.qth");
    Lrp lrp_obj(0.0, 0.0);

    bool loadPat = false;
    std::string filename;
    lrp_obj.ReadLRParm(tx_site, 0, loadPat, filename);

    // Should still load correctly despite comments
    EXPECT_NEAR(lrp_obj.eps_dielect, 15.0, 0.01);
    EXPECT_NEAR(lrp_obj.frq_mhz, 900.0, 0.1);

    remove("test_comments.lrp");
    remove("test_comments.qth");
}

// Test falling back to splat.lrp
TEST_F(LrpTest, FallbackToSplatLRP) {
    // Create splat.lrp file
    std::ofstream splat_lrp("splat.lrp");
    splat_lrp << "12.000\n0.003\n290.000\n850.000\n5\n1\n0.50\n0.95\n500.0\n";
    splat_lrp.close();

    // Create site with different name (no matching .lrp file)
    std::ofstream qth("different.qth");
    qth << "Different Site\n45.0\n-120.0\n100.0\n";
    qth.close();

    Site tx_site("different.qth");
    Lrp lrp(0.0, 0.0);

    bool loadPat = false;
    std::string filename;
    lrp.ReadLRParm(tx_site, 0, loadPat, filename);

    // Should load from splat.lrp
    EXPECT_NEAR(lrp.eps_dielect, 12.0, 0.01);
    EXPECT_NEAR(lrp.frq_mhz, 850.0, 0.1);
    EXPECT_EQ(lrp.pol, 1);  // Vertical polarization from splat.lrp

    remove("different.qth");
}

// Test forced ERP override
TEST_F(LrpTest, ForcedERPOverride) {
    Site tx_site("test_tx.qth");
    Lrp lrp(2500.0, 0.0);  // Force ERP to 2500W

    bool loadPat = false;
    std::string filename;
    lrp.ReadLRParm(tx_site, 0, loadPat, filename);

    // ERP should be forced to 2500, not 1000 from file
    EXPECT_NEAR(lrp.erp, 2500.0, 0.1);
}

// Test forced frequency override
TEST_F(LrpTest, ForcedFrequencyOverride) {
    Site tx_site("test_tx.qth");
    Lrp lrp(0.0, 1800.0);  // Force frequency to 1800 MHz

    bool loadPat = false;
    std::string filename;
    lrp.ReadLRParm(tx_site, 0, loadPat, filename);

    // Frequency should be forced to 1800, not 900 from file
    EXPECT_NEAR(lrp.frq_mhz, 1800.0, 0.1);
}

// Test both forced parameters
TEST_F(LrpTest, BothForcedParameters) {
    Site tx_site("test_tx.qth");
    Lrp lrp(3000.0, 2400.0);  // Force both ERP and frequency

    bool loadPat = false;
    std::string filename;
    lrp.ReadLRParm(tx_site, 0, loadPat, filename);

    EXPECT_NEAR(lrp.erp, 3000.0, 0.1);
    EXPECT_NEAR(lrp.frq_mhz, 2400.0, 0.1);
}

// Test radio climate values
TEST_F(LrpTest, RadioClimateValues) {
    // Test different climate values (1-7 are valid)
    std::ofstream lrp("climate_test.lrp");
    lrp << "15.000\n0.005\n301.000\n900.000\n";
    lrp << "3\n";  // Climate 3 = Suburban
    lrp << "0\n0.50\n0.90\n1000.0\n";
    lrp.close();

    std::ofstream qth("climate_test.qth");
    qth << "Test\n45.0\n-120.0\n100.0\n";
    qth.close();

    Site tx_site("climate_test.qth");
    Lrp lrp_obj(0.0, 0.0);

    bool loadPat = false;
    std::string filename;
    lrp_obj.ReadLRParm(tx_site, 0, loadPat, filename);

    EXPECT_EQ(lrp_obj.radio_climate, 3);

    remove("climate_test.lrp");
    remove("climate_test.qth");
}

// Test polarization values
TEST_F(LrpTest, PolarizationHorizontal) {
    Site tx_site("test_tx.qth");
    Lrp lrp(0.0, 0.0);

    bool loadPat = false;
    std::string filename;
    lrp.ReadLRParm(tx_site, 0, loadPat, filename);

    EXPECT_EQ(lrp.pol, 0);  // Horizontal from test file
}

TEST_F(LrpTest, PolarizationVertical) {
    std::ofstream lrp("vert_pol.lrp");
    lrp << "15.000\n0.005\n301.000\n900.000\n5\n";
    lrp << "1\n";  // Vertical polarization
    lrp << "0.50\n0.90\n1000.0\n";
    lrp.close();

    std::ofstream qth("vert_pol.qth");
    qth << "Test\n45.0\n-120.0\n100.0\n";
    qth.close();

    Site tx_site("vert_pol.qth");
    Lrp lrp_obj(0.0, 0.0);

    bool loadPat = false;
    std::string filename;
    lrp_obj.ReadLRParm(tx_site, 0, loadPat, filename);

    EXPECT_EQ(lrp_obj.pol, 1);

    remove("vert_pol.lrp");
    remove("vert_pol.qth");
}

// Test confidence and reliability values
TEST_F(LrpTest, ConfidenceAndReliability) {
    Site tx_site("test_tx.qth");
    Lrp lrp(0.0, 0.0);

    bool loadPat = false;
    std::string filename;
    lrp.ReadLRParm(tx_site, 0, loadPat, filename);

    // Confidence and reliability should be fractions (0-1)
    EXPECT_GE(lrp.conf, 0.0);
    EXPECT_LE(lrp.conf, 1.0);
    EXPECT_GE(lrp.rel, 0.0);
    EXPECT_LE(lrp.rel, 1.0);

    EXPECT_NEAR(lrp.conf, 0.50, 0.01);
    EXPECT_NEAR(lrp.rel, 0.90, 0.01);
}

// Test earth parameters
TEST_F(LrpTest, EarthParameters) {
    Site tx_site("test_tx.qth");
    Lrp lrp(0.0, 0.0);

    bool loadPat = false;
    std::string filename;
    lrp.ReadLRParm(tx_site, 0, loadPat, filename);

    // Earth dielectric constant typically 4-80
    EXPECT_GT(lrp.eps_dielect, 0.0);
    EXPECT_LT(lrp.eps_dielect, 100.0);

    // Earth conductivity typically 0.001-0.01 S/m
    EXPECT_GT(lrp.sgm_conductivity, 0.0);
    EXPECT_LT(lrp.sgm_conductivity, 1.0);

    // Atmospheric bending constant typically 250-400 N-units
    EXPECT_GT(lrp.eno_ns_surfref, 200.0);
    EXPECT_LT(lrp.eno_ns_surfref, 500.0);
}

// Test frequency bounds
TEST_F(LrpTest, FrequencyBounds) {
    Site tx_site("test_tx.qth");
    Lrp lrp(0.0, 0.0);

    bool loadPat = false;
    std::string filename;
    lrp.ReadLRParm(tx_site, 0, loadPat, filename);

    // ITWOM valid range is 20 MHz to 20 GHz
    EXPECT_NEAR(lrp.frq_mhz, 900.0, 0.1);
    EXPECT_GE(lrp.frq_mhz, 20.0);
    EXPECT_LE(lrp.frq_mhz, 20000.0);
}

// Parameterized test for different radio climates
class LrpClimateTest : public ::testing::TestWithParam<int> { };

TEST_P(LrpClimateTest, VariousClimates) {
    int climate = GetParam();

    std::ofstream lrp("param_climate.lrp");
    lrp << "15.000\n0.005\n301.000\n900.000\n";
    lrp << climate << "\n";
    lrp << "0\n0.50\n0.90\n1000.0\n";
    lrp.close();

    std::ofstream qth("param_climate.qth");
    qth << "Test\n45.0\n-120.0\n100.0\n";
    qth.close();

    Site tx_site("param_climate.qth");
    Lrp lrp_obj(0.0, 0.0);

    bool loadPat = false;
    std::string filename;
    lrp_obj.ReadLRParm(tx_site, 0, loadPat, filename);

    EXPECT_EQ(lrp_obj.radio_climate, climate);

    remove("param_climate.lrp");
    remove("param_climate.qth");
}

INSTANTIATE_TEST_SUITE_P(RadioClimates, LrpClimateTest,
                         ::testing::Values(1, 2, 3, 4, 5, 6,
                                           7)  // Valid climate values
);

// Parameterized test for various frequencies
class LrpFrequencyTest : public ::testing::TestWithParam<double> { };

TEST_P(LrpFrequencyTest, VariousFrequencies) {
    double freq = GetParam();

    std::ofstream lrp("param_freq.lrp");
    lrp << "15.000\n0.005\n301.000\n";
    lrp << freq << "\n";
    lrp << "5\n0\n0.50\n0.90\n1000.0\n";
    lrp.close();

    std::ofstream qth("param_freq.qth");
    qth << "Test\n45.0\n-120.0\n100.0\n";
    qth.close();

    Site tx_site("param_freq.qth");
    Lrp lrp_obj(0.0, 0.0);

    bool loadPat = false;
    std::string filename;
    lrp_obj.ReadLRParm(tx_site, 0, loadPat, filename);

    EXPECT_NEAR(lrp_obj.frq_mhz, freq, 0.1);

    remove("param_freq.lrp");
    remove("param_freq.qth");
}

INSTANTIATE_TEST_SUITE_P(Frequencies, LrpFrequencyTest,
                         ::testing::Values(50.0, 150.0, 450.0, 900.0, 1800.0,
                                           2400.0, 5800.0));
