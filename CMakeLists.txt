cmake_minimum_required(VERSION 3.16)
project(splat VERSION 2.0 LANGUAGES C CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required libraries
find_package(PNG REQUIRED)
find_package(JPEG REQUIRED)
find_package(GDAL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(BZip2 REQUIRED)

# Include directories
include_directories(${PNG_INCLUDE_DIRS} ${JPEG_INCLUDE_DIRS} ${GDAL_INCLUDE_DIRS} ${ZLIB_INCLUDE_DIRS} ${BZIP2_INCLUDE_DIRS})

# Source files (example, adjust based on actual project structure)
file(GLOB_RECURSE SOURCES src/*.cpp src/*.c)

# Create main executable
add_executable(splat ${SOURCES})
target_link_libraries(splat ${PNG_LIBRARIES} ${JPEG_LIBRARIES} ${GDAL_LIBRARIES} ${ZLIB_LIBRARIES} ${BZIP2_LIBRARIES})

# Installation
install(TARGETS splat DESTINATION bin)

# Enable testing
enable_testing()

# Find or fetch GoogleTest and GoogleMock. Stick to 1.12 as long as we're C++11 compatible.
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.12.1
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Test source files (example, adjust paths as needed)
file(GLOB_RECURSE TEST_SOURCES tests/*.cpp)

# Create test executable
add_executable(splat_tests ${TEST_SOURCES})
target_link_libraries(splat_tests GTest::gtest_main GTest::gmock_main ${PNG_LIBRARIES} ${JPEG_LIBRARIES} ${GDAL_LIBRARIES} ${ZLIB_LIBRARIES} ${BZIP2_LIBRARIES})

# Include project source directories for tests
target_include_directories(splat_tests PRIVATE src)

# Add tests to CTest
include(GoogleTest)
gtest_discover_tests(splat_tests)

# Installation of test executable (optional)
install(TARGETS splat_tests DESTINATION bin)
