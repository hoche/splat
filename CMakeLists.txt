cmake_minimum_required(VERSION 3.16)
project(splat VERSION 2.0 LANGUAGES C CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_BUILD_TYPE "Debug")

# Find required libraries
find_package(ZLIB REQUIRED)
find_package(BZip2 REQUIRED)
find_package(Threads REQUIRED)

# Find optional libraries for graphics support
find_package(PNG)
find_package(JPEG)
find_package(GDAL)

# Define preprocessor macros for conditional compilation based on what was found
if(PNG_FOUND)
    add_compile_definitions(HAVE_LIBPNG)
    message(STATUS "PNG support enabled")
else()
    message(STATUS "PNG support disabled - libpng not found")
endif()

if(JPEG_FOUND)
    add_compile_definitions(HAVE_LIBJPEG)
    message(STATUS "JPEG support enabled")
else()
    message(STATUS "JPEG support disabled - libjpeg not found")
endif()

if(GDAL_FOUND)
    add_compile_definitions(HAVE_LIBGDAL)
    message(STATUS "GDAL support enabled")
else()
    message(STATUS "GDAL support disabled - libgdal not found")
endif()

# Explicitly list all source files for the main executable
# This is preferred over GLOB_RECURSE because CMake will automatically
# detect when new files are added to the list
set(SPLAT_SOURCES
    src/anf.cpp
    src/antenna_pattern.cpp
    src/boundary_file.cpp
    src/city_file.cpp
    src/dem.cpp
    src/elevation_map.cpp
    src/gnuplot.cpp
    src/image.cpp
    src/imagewriter.cpp
    src/itwom3.0.cpp
    src/json.cpp
    src/kml.cpp
    src/lrp.cpp
    src/main.cpp
    src/path.cpp
    src/region.cpp
    src/report.cpp
    src/sdf_bz.cpp
    src/sdf.cpp
    src/site.cpp
    src/splat_run.cpp
    src/sysutil.cpp
    src/udt.cpp
    src/utilities.cpp
    src/workqueue.cpp
    src/zip.c
)

# Create main executable
add_executable(splat ${SPLAT_SOURCES})

# Use target-specific include directories instead of global include_directories
target_include_directories(splat PRIVATE
    ${ZLIB_INCLUDE_DIRS}
    ${BZIP2_INCLUDE_DIRS}
)

# Add optional library includes if found
if(PNG_FOUND)
    target_include_directories(splat PRIVATE ${PNG_INCLUDE_DIRS})
endif()
if(JPEG_FOUND)
    target_include_directories(splat PRIVATE ${JPEG_INCLUDE_DIRS})
endif()
if(GDAL_FOUND)
    target_include_directories(splat PRIVATE ${GDAL_INCLUDE_DIRS})
endif()

# Link libraries to the executable
target_link_libraries(splat PRIVATE
    ${ZLIB_LIBRARIES}
    ${BZIP2_LIBRARIES}
    Threads::Threads
)

# Link optional libraries if found
if(PNG_FOUND)
    target_link_libraries(splat PRIVATE ${PNG_LIBRARIES})
endif()
if(JPEG_FOUND)
    target_link_libraries(splat PRIVATE ${JPEG_LIBRARIES})
endif()
if(GDAL_FOUND)
    target_link_libraries(splat PRIVATE ${GDAL_LIBRARIES})
endif()

# Installation
install(TARGETS splat DESTINATION bin)

# ==============================================================================
# Testing Configuration
# ==============================================================================

# Enable testing
enable_testing()

# Find or fetch GoogleTest and GoogleMock
# Stick to 1.17 as long as we're C++17 compatible
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Explicitly list test source files
set(TEST_SOURCES
    tests/antenna_pattern_test.cpp
    tests/image_output_test.cpp
    tests/input_file_parsing_test.cpp
    tests/itwom_test.cpp
    tests/site_test.cpp
    tests/utilities_test.cpp
    tests/lrp_test.cpp
    tests/sdf_test.cpp
    tests/sdf_bz_test.cpp
    tests/kml_test.cpp
)

# List source files needed by tests (excluding main.cpp)
# Tests need access to the implementation but not the main() function
set(TEST_REQUIRED_SOURCES
    src/anf.cpp
    src/antenna_pattern.cpp
    src/boundary_file.cpp
    src/city_file.cpp
    src/dem.cpp
    src/elevation_map.cpp
    src/gnuplot.cpp
    src/image.cpp
    src/imagewriter.cpp
    src/itwom3.0.cpp
    src/json.cpp
    src/kml.cpp
    src/lrp.cpp
    # NOTE: src/main.cpp is intentionally excluded - tests use gtest_main instead
    src/path.cpp
    src/region.cpp
    src/report.cpp
    src/sdf_bz.cpp
    src/sdf.cpp
    src/site.cpp
    src/splat_run.cpp
    src/sysutil.cpp
    src/udt.cpp
    src/utilities.cpp
    src/workqueue.cpp
    src/zip.c
)

# Create test executable
add_executable(splat_tests ${TEST_SOURCES} ${TEST_REQUIRED_SOURCES})

# Link test libraries
target_link_libraries(splat_tests PRIVATE
    GTest::gtest_main
    GTest::gmock_main
    ${ZLIB_LIBRARIES}
    ${BZIP2_LIBRARIES}
    Threads::Threads
)

# Link optional libraries to tests if found
if(PNG_FOUND)
    target_link_libraries(splat_tests PRIVATE ${PNG_LIBRARIES})
endif()
if(JPEG_FOUND)
    target_link_libraries(splat_tests PRIVATE ${JPEG_LIBRARIES})
endif()
if(GDAL_FOUND)
    target_link_libraries(splat_tests PRIVATE ${GDAL_LIBRARIES})
endif()

# Include directories for tests
target_include_directories(splat_tests PRIVATE
    src
    ${ZLIB_INCLUDE_DIRS}
    ${BZIP2_INCLUDE_DIRS}
)

# Add optional library includes to tests if found
if(PNG_FOUND)
    target_include_directories(splat_tests PRIVATE ${PNG_INCLUDE_DIRS})
endif()
if(JPEG_FOUND)
    target_include_directories(splat_tests PRIVATE ${JPEG_INCLUDE_DIRS})
endif()
if(GDAL_FOUND)
    target_include_directories(splat_tests PRIVATE ${GDAL_INCLUDE_DIRS})
endif()

# Discover and register tests with CTest
include(GoogleTest)
gtest_discover_tests(splat_tests)

# Installation of test executable (optional)
install(TARGETS splat_tests DESTINATION bin)
