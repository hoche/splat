#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "itwom3.0.h"

// Mock class for dependencies (e.g., terrain data access)
class MockTerrainData {
public:
    MOCK_METHOD2(get_elevation, double(double lat, double lon));
    MOCK_METHOD1(get_terrain_profile, bool(std::vector<double>& profile));
};

// Test fixture for ITWOM propagation model
class ITWOMTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize common test data
        elev_data = {100.0, 200.0, 150.0}; // Sample elevation profile
        freq_mhz = 1000.0; // 1 GHz
        distance_km = 10.0; // 10 km
    }

    std::vector<double> elev_data;
    double freq_mhz;
    double distance_km;
    MockTerrainData mock_terrain;
};

// Mock function to simulate point_to_point (adjust signature as needed)
double point_to_point(const std::vector<double>& elev, double distance_km, double freq_mhz, MockTerrainData& terrain) {
    // Simplified mock implementation for testing
    if (elev.empty() || distance_km <= 0.0 || freq_mhz < 20.0 || freq_mhz > 20000.0) {
        return -1.0; // Invalid input
    }
    // Simulate terrain profile access
    std::vector<double> profile;
    if (!terrain.get_terrain_profile(profile)) {
        return -1.0;
    }
    // Dummy loss calculation (replace with actual ITWOM logic)
    double loss = 100.0 + distance_km * 10.0; // Example formula
    return loss;
}

// Test valid point-to-point propagation loss
TEST_F(ITWOMTest, PointToPointValidInput) {
    // Mock terrain profile access
    std::vector<double> mock_profile = {100.0, 150.0, 200.0};
    EXPECT_CALL(mock_terrain, get_terrain_profile(testing::_))
        .WillOnce(testing::DoAll(
            testing::SetArgReferee<0>(mock_profile),
            testing::Return(true)));

    // Call point_to_point
    double loss = point_to_point(elev_data, distance_km, freq_mhz, mock_terrain);
    EXPECT_GT(loss, 0.0) << "Propagation loss should be positive";
    EXPECT_LT(loss, 200.0) << "Propagation loss should be within reasonable bounds";
}

// Test zero distance
TEST_F(ITWOMTest, PointToPointZeroDistance) {
    distance_km = 0.0;
    // Mock terrain profile (should not be called due to invalid distance)
    EXPECT_CALL(mock_terrain, get_terrain_profile(testing::_)).Times(0);

    // Call point_to_point
    double loss = point_to_point(elev_data, distance_km, freq_mhz, mock_terrain);
    EXPECT_EQ(loss, -1.0) << "Propagation loss should be invalid for zero distance";
}

// Test out-of-range frequency
TEST_F(ITWOMTest, PointToPointInvalidFrequency) {
    freq_mhz = 10.0; // Below 20 MHz
    // Mock terrain profile (should not be called due to invalid frequency)
    EXPECT_CALL(mock_terrain, get_terrain_profile(testing::_)).Times(0);

    // Call point_to_point
    double loss = point_to_point(elev_data, distance_km, freq_mhz, mock_terrain);
    EXPECT_EQ(loss, -1.0) << "Propagation loss should be invalid for out-of-range frequency";
}

// Test empty elevation profile
TEST_F(ITWOMTest, PointToPointEmptyElevation) {
    elev_data.clear();
    // Mock terrain profile (should not be called due to empty elevation)
    EXPECT_CALL(mock_terrain, get_terrain_profile(testing::_)).Times(0);

    // Call point_to_point
    double loss = point_to_point(elev_data, distance_km, freq_mhz, mock_terrain);
    EXPECT_EQ(loss, -1.0) << "Propagation loss should be invalid for empty elevation profile";
}

// Test terrain data access failure
TEST_F(ITWOMTest, PointToPointTerrainFailure) {
    // Mock terrain profile access failure
    EXPECT_CALL(mock_terrain, get_terrain_profile(testing::_))
        .WillOnce(testing::Return(false));

    // Call point_to_point
    double loss = point_to_point(elev_data, distance_km, freq_mhz, mock_terrain);
    EXPECT_EQ(loss, -1.0) << "Propagation loss should be invalid if terrain access fails";
}

// Parameterized test for different distances
class ITWOMDistanceTest : public ITWOMTest, public ::testing::WithParamInterface<double> {
protected:
    void SetUp() override {
        ITWOMTest::SetUp();
        distance_km = GetParam();
    }
};

// Test point-to-point with various distances
TEST_P(ITWOMDistanceTest, PointToPointVariousDistances) {
    // Mock terrain profile access
    std::vector<double> mock_profile = {100.0, 150.0, 200.0};
    EXPECT_CALL(mock_terrain, get_terrain_profile(testing::_))
        .WillOnce(testing::DoAll(
            testing::SetArgReferee<0>(mock_profile),
            testing::Return(true)));

    // Call point_to_point
    double loss = point_to_point(elev_data, distance_km, freq_mhz, mock_terrain);
    EXPECT_GT(loss, 0.0) << "Propagation loss should be positive for distance " << distance_km;
    EXPECT_LT(loss, 1000.0) << "Propagation loss should be reasonable for distance " << distance_km;
}

// Instantiate parameterized test with different distances
INSTANTIATE_TEST_SUITE_P(
    Distances,
    ITWOMDistanceTest,
    ::testing::Values(1.0, 10.0, 100.0, 500.0)); // Various distances in km

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
