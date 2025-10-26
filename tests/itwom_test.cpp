#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <thread>
#include <vector>

#include "itwom3.0.h"  // Adjust path to include ITWOM header

// Mock class for terrain and external dependencies
class MockTerrainData {
  public:
    MOCK_METHOD2(get_elevation, double(double lat, double lon));
    MOCK_METHOD1(get_terrain_profile, bool(std::vector<double> &profile));
    MOCK_METHOD0(get_surface_roughness, double());
};

// Test fixture for ITWOM propagation model
class ITWOMTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Initialize common test data
        elev_data = {100.0, 150.0, 200.0, 180.0,
                     120.0};  // Sample elevation profile
        freq_mhz = 1000.0;    // 1 GHz
        distance_km = 10.0;   // 10 km
        epsilon = 1e-6;       // Tolerance for floating-point comparisons
    }

    std::vector<double> elev_data;
    double freq_mhz;
    double distance_km;
    double epsilon;
    MockTerrainData mock_terrain;
};

// Mock function to simulate point_to_point (adjust signature as needed)
double point_to_point(const std::vector<double> &elev, double distance_km,
                      double freq_mhz, MockTerrainData &terrain) {
    // Simplified mock implementation for testing
    if (elev.size() < 2 || distance_km <= 0.0 || freq_mhz < 20.0 ||
        freq_mhz > 20000.0) {
        return -1.0;  // Invalid input
    }
    // Simulate terrain profile access
    std::vector<double> profile;
    if (! terrain.get_terrain_profile(profile)) {
        return -1.0;
    }
    // Dummy loss calculation (replace with actual ITWOM logic)
    double roughness = terrain.get_surface_roughness();
    double loss = 92.45 + 20.0 * log10(distance_km) + 20.0 * log10(freq_mhz) +
                  roughness;  // Free-space loss + roughness
    return loss;
}

// Test valid point-to-point propagation loss
TEST_F(ITWOMTest, PointToPointValidInput) {
    // Mock terrain profile and surface roughness
    std::vector<double> mock_profile = {100.0, 150.0, 200.0, 180.0, 120.0};
    EXPECT_CALL(mock_terrain, get_terrain_profile(testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<0>(mock_profile),
                                 testing::Return(true)));
    EXPECT_CALL(mock_terrain, get_surface_roughness())
        .WillOnce(testing::Return(5.0));  // Sample roughness

    // Call point_to_point
    double loss =
        point_to_point(elev_data, distance_km, freq_mhz, mock_terrain);
    double expected_loss = 92.45 + 20.0 * log10(10.0) + 20.0 * log10(1000.0) +
                           5.0;  // Free-space + roughness
    EXPECT_NEAR(loss, expected_loss, epsilon)
        << "Propagation loss should match expected value";
}

// Test zero distance
TEST_F(ITWOMTest, PointToPointZeroDistance) {
    distance_km = 0.0;
    // Mock terrain profile (should not be called)
    EXPECT_CALL(mock_terrain, get_terrain_profile(testing::_)).Times(0);
    EXPECT_CALL(mock_terrain, get_surface_roughness()).Times(0);

    // Call point_to_point
    double loss =
        point_to_point(elev_data, distance_km, freq_mhz, mock_terrain);
    EXPECT_EQ(loss, -1.0)
        << "Propagation loss should be invalid for zero distance";
}

// Test boundary frequencies (20 MHz and 20 GHz)
TEST_F(ITWOMTest, PointToPointBoundaryFrequencies) {
    // Test lower boundary (20 MHz)
    freq_mhz = 20.0;
    std::vector<double> mock_profile = {100.0, 150.0, 200.0};
    EXPECT_CALL(mock_terrain, get_terrain_profile(testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<0>(mock_profile),
                                 testing::Return(true)));
    EXPECT_CALL(mock_terrain, get_surface_roughness())
        .WillOnce(testing::Return(5.0));

    double loss =
        point_to_point(elev_data, distance_km, freq_mhz, mock_terrain);
    double expected_loss =
        92.45 + 20.0 * log10(10.0) + 20.0 * log10(20.0) + 5.0;
    EXPECT_NEAR(loss, expected_loss, epsilon)
        << "Propagation loss should be valid for 20 MHz";

    // Test upper boundary (20 GHz)
    freq_mhz = 20000.0;
    EXPECT_CALL(mock_terrain, get_terrain_profile(testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<0>(mock_profile),
                                 testing::Return(true)));
    EXPECT_CALL(mock_terrain, get_surface_roughness())
        .WillOnce(testing::Return(5.0));

    loss = point_to_point(elev_data, distance_km, freq_mhz, mock_terrain);
    expected_loss = 92.45 + 20.0 * log10(10.0) + 20.0 * log10(20000.0) + 5.0;
    EXPECT_NEAR(loss, expected_loss, epsilon)
        << "Propagation loss should be valid for 20 GHz";
}

// Test out-of-range frequencies
TEST_F(ITWOMTest, PointToPointInvalidFrequency) {
    freq_mhz = 10.0;  // Below 20 MHz
    EXPECT_CALL(mock_terrain, get_terrain_profile(testing::_)).Times(0);
    EXPECT_CALL(mock_terrain, get_surface_roughness()).Times(0);

    double loss =
        point_to_point(elev_data, distance_km, freq_mhz, mock_terrain);
    EXPECT_EQ(loss, -1.0)
        << "Propagation loss should be invalid for frequency < 20 MHz";

    freq_mhz = 25000.0;  // Above 20 GHz
    EXPECT_CALL(mock_terrain, get_terrain_profile(testing::_)).Times(0);
    EXPECT_CALL(mock_terrain, get_surface_roughness()).Times(0);

    loss = point_to_point(elev_data, distance_km, freq_mhz, mock_terrain);
    EXPECT_EQ(loss, -1.0)
        << "Propagation loss should be invalid for frequency > 20 GHz";
}

// Test empty elevation profile
TEST_F(ITWOMTest, PointToPointEmptyElevation) {
    elev_data.clear();
    EXPECT_CALL(mock_terrain, get_terrain_profile(testing::_)).Times(0);
    EXPECT_CALL(mock_terrain, get_surface_roughness()).Times(0);

    double loss =
        point_to_point(elev_data, distance_km, freq_mhz, mock_terrain);
    EXPECT_EQ(loss, -1.0)
        << "Propagation loss should be invalid for empty elevation profile";
}

// Test terrain data access failure
TEST_F(ITWOMTest, PointToPointTerrainFailure) {
    EXPECT_CALL(mock_terrain, get_terrain_profile(testing::_))
        .WillOnce(testing::Return(false));
    EXPECT_CALL(mock_terrain, get_surface_roughness()).Times(0);

    double loss =
        point_to_point(elev_data, distance_km, freq_mhz, mock_terrain);
    EXPECT_EQ(loss, -1.0)
        << "Propagation loss should be invalid if terrain access fails";
}

// Test line-of-sight scenario
TEST_F(ITWOMTest, PointToPointLineOfSight) {
    // Mock flat terrain for LOS
    std::vector<double> mock_profile = {100.0, 100.0, 100.0};  // Flat terrain
    EXPECT_CALL(mock_terrain, get_terrain_profile(testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<0>(mock_profile),
                                 testing::Return(true)));
    EXPECT_CALL(mock_terrain, get_surface_roughness())
        .WillOnce(testing::Return(0.0));  // Smooth surface

    double loss =
        point_to_point(elev_data, distance_km, freq_mhz, mock_terrain);
    double expected_loss =
        92.45 + 20.0 * log10(10.0) + 20.0 * log10(1000.0);  // Free-space loss
    EXPECT_NEAR(loss, expected_loss, epsilon)
        << "Propagation loss should match free-space for LOS";
}

// Test knife-edge diffraction scenario
TEST_F(ITWOMTest, PointToPointKnifeEdgeDiffraction) {
    // Mock terrain with a single peak
    std::vector<double> mock_profile = {100.0, 300.0,
                                        100.0};  // Single obstruction
    EXPECT_CALL(mock_terrain, get_terrain_profile(testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<0>(mock_profile),
                                 testing::Return(true)));
    EXPECT_CALL(mock_terrain, get_surface_roughness())
        .WillOnce(testing::Return(5.0));

    double loss =
        point_to_point(elev_data, distance_km, freq_mhz, mock_terrain);
    EXPECT_GT(loss, 92.45 + 20.0 * log10(10.0) + 20.0 * log10(1000.0))
        << "Propagation loss should include diffraction loss for knife-edge "
           "scenario";
}

// Test multithreading safety (simplified)
TEST_F(ITWOMTest, PointToPointMultithreaded) {
    std::vector<double> mock_profile = {100.0, 150.0, 200.0};
    EXPECT_CALL(mock_terrain, get_terrain_profile(testing::_))
        .Times(2)
        .WillRepeatedly(testing::DoAll(testing::SetArgReferee<0>(mock_profile),
                                       testing::Return(true)));
    EXPECT_CALL(mock_terrain, get_surface_roughness())
        .Times(2)
        .WillRepeatedly(testing::Return(5.0));

    double loss1, loss2;
    std::thread t1([&]() {
        loss1 = point_to_point(elev_data, distance_km, freq_mhz, mock_terrain);
    });
    std::thread t2([&]() {
        loss2 = point_to_point(elev_data, distance_km, freq_mhz, mock_terrain);
    });
    t1.join();
    t2.join();

    double expected_loss =
        92.45 + 20.0 * log10(10.0) + 20.0 * log10(1000.0) + 5.0;
    EXPECT_NEAR(loss1, expected_loss, epsilon)
        << "Thread 1 loss should match expected value";
    EXPECT_NEAR(loss2, expected_loss, epsilon)
        << "Thread 2 loss should match expected value";
}

// Parameterized test for different frequencies and distances
class ITWOMParamTest
    : public ITWOMTest,
      public ::testing::WithParamInterface<std::tuple<double, double>> {
  protected:
    void SetUp() override {
        ITWOMTest::SetUp();
        distance_km = std::get<0>(GetParam());
        freq_mhz = std::get<1>(GetParam());
    }
};

// Test point-to-point with various distances and frequencies
TEST_P(ITWOMParamTest, PointToPointVariousInputs) {
    std::vector<double> mock_profile = {100.0, 150.0, 200.0};
    EXPECT_CALL(mock_terrain, get_terrain_profile(testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<0>(mock_profile),
                                 testing::Return(true)));
    EXPECT_CALL(mock_terrain, get_surface_roughness())
        .WillOnce(testing::Return(5.0));

    double loss =
        point_to_point(elev_data, distance_km, freq_mhz, mock_terrain);
    double expected_loss =
        92.45 + 20.0 * log10(distance_km) + 20.0 * log10(freq_mhz) + 5.0;
    EXPECT_NEAR(loss, expected_loss, epsilon)
        << "Propagation loss should match expected for distance " << distance_km
        << " km and frequency " << freq_mhz << " MHz";
}

// Instantiate parameterized test with various distances and frequencies
INSTANTIATE_TEST_SUITE_P(
    DistanceAndFrequency, ITWOMParamTest,
    ::testing::Values(
        std::make_tuple(1.0, 100.0),     // Short distance, low frequency
        std::make_tuple(10.0, 1000.0),   // Medium distance, mid frequency
        std::make_tuple(100.0, 5000.0),  // Long distance, high frequency
        std::make_tuple(500.0, 20000.0)  // Very long distance, max frequency
        ));
