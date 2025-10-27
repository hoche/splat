cmake_minimum_required(VERSION 3.16)

# Prefer Clang if available (better sanitizer support), but fall back to GCC
# This can be overridden with: CC=gcc CXX=g++ cmake ..
# Note: Must be set before project() command
if(NOT DEFINED ENV{CC} AND NOT DEFINED ENV{CXX} AND NOT DEFINED CMAKE_C_COMPILER AND NOT DEFINED CMAKE_CXX_COMPILER)
    find_program(CLANG_EXECUTABLE NAMES clang)
    find_program(CLANGXX_EXECUTABLE NAMES clang++)

    if(CLANG_EXECUTABLE AND CLANGXX_EXECUTABLE)
        set(CMAKE_C_COMPILER ${CLANG_EXECUTABLE} CACHE FILEPATH "C compiler" FORCE)
        set(CMAKE_CXX_COMPILER ${CLANGXX_EXECUTABLE} CACHE FILEPATH "C++ compiler" FORCE)
        message(STATUS "Using Clang compiler (for better sanitizer support)")
    else()
        message(STATUS "Clang not found, using default compiler")
    endif()
endif()

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

# ==============================================================================
# Sanitizer and Static Analysis Targets
# ==============================================================================

# Function to create sanitizer build targets
function(add_sanitizer_target TARGET_NAME SANITIZER_NAME SANITIZER_FLAGS)
    add_custom_target(${TARGET_NAME}
        COMMAND ${CMAKE_COMMAND} -E echo "Building with ${SANITIZER_NAME}..."
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/${TARGET_NAME}
        COMMAND ${CMAKE_COMMAND}
            "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}"
            "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
            "-DCMAKE_CXX_FLAGS=-fsanitize=${SANITIZER_FLAGS} -fno-omit-frame-pointer -g -O1"
            "-DCMAKE_C_FLAGS=-fsanitize=${SANITIZER_FLAGS} -fno-omit-frame-pointer -g -O1"
            "-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=${SANITIZER_FLAGS}"
            -S ${CMAKE_SOURCE_DIR}
            -B ${CMAKE_BINARY_DIR}/${TARGET_NAME}
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}/${TARGET_NAME} --target splat_tests
        COMMAND ${CMAKE_COMMAND} -E echo "Running tests with ${SANITIZER_NAME}..."
        COMMAND cd ${CMAKE_BINARY_DIR}/${TARGET_NAME} && ./splat_tests
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running ${SANITIZER_NAME}"
        VERBATIM
    )
endfunction()

# AddressSanitizer - detects memory errors (use-after-free, buffer overflows, etc.)
add_sanitizer_target(asan "AddressSanitizer (ASan)" "address")

# UndefinedBehaviorSanitizer - detects undefined behavior
add_sanitizer_target(ubsan "UndefinedBehaviorSanitizer (UBSan)" "undefined")

# ThreadSanitizer - detects data races and deadlocks
# Uses suppression file to filter known false positives from GDAL library
add_custom_target(tsan
    COMMAND ${CMAKE_COMMAND} -E echo "Building with ThreadSanitizer..."
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/tsan
    COMMAND ${CMAKE_COMMAND}
        "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}"
        "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
        "-DCMAKE_CXX_FLAGS=-fsanitize=thread -fno-omit-frame-pointer -g -O1"
        "-DCMAKE_C_FLAGS=-fsanitize=thread -fno-omit-frame-pointer -g -O1"
        "-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=thread"
        -S ${CMAKE_SOURCE_DIR}
        -B ${CMAKE_BINARY_DIR}/tsan
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}/tsan --target splat_tests
    COMMAND ${CMAKE_COMMAND} -E echo "Running tests with ThreadSanitizer..."
    COMMAND ${CMAKE_SOURCE_DIR}/run_tsan.sh ${CMAKE_BINARY_DIR}/tsan/splat_tests
    COMMENT "Running ThreadSanitizer"
)

# MemorySanitizer - detects uninitialized memory reads (Clang only)
# Note: MSan requires Clang and ALL dependencies compiled with MSan
# This is typically not feasible in standard development environments
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_custom_target(msan
        COMMAND ${CMAKE_COMMAND} -E echo "=== MemorySanitizer MSan Information ==="
        COMMAND ${CMAKE_COMMAND} -E echo ""
        COMMAND ${CMAKE_COMMAND} -E echo "MSan requires ALL code including system libraries to be"
        COMMAND ${CMAKE_COMMAND} -E echo "compiled with -fsanitize=memory. In most environments,"
        COMMAND ${CMAKE_COMMAND} -E echo "system libraries like GDAL, PROJ, libpng are NOT compiled"
        COMMAND ${CMAKE_COMMAND} -E echo "with MSan, which prevents MSan from running successfully."
        COMMAND ${CMAKE_COMMAND} -E echo ""
        COMMAND ${CMAKE_COMMAND} -E echo "This is a known limitation of MSan, not a bug in SPLAT."
        COMMAND ${CMAKE_COMMAND} -E echo ""
        COMMAND ${CMAKE_COMMAND} -E echo "For this reason, the msan target is informational only."
        COMMAND ${CMAKE_COMMAND} -E echo "See TESTING.md for full details on MSan limitations."
        COMMAND ${CMAKE_COMMAND} -E echo ""
        COMMAND ${CMAKE_COMMAND} -E echo "If you have an MSan-instrumented environment, build with:"
        COMMAND ${CMAKE_COMMAND} -E echo "  CC=clang CXX=clang++ cmake -B build-msan"
        COMMAND ${CMAKE_COMMAND} -E echo "    -DCMAKE_CXX_FLAGS=-fsanitize=memory"
        COMMAND ${CMAKE_COMMAND} -E echo "    -DCMAKE_C_FLAGS=-fsanitize=memory"
        COMMAND ${CMAKE_COMMAND} -E echo "    -DCMAKE_EXE_LINKER_FLAGS=-fsanitize=memory"
        COMMAND ${CMAKE_COMMAND} -E echo "  cmake --build build-msan"
        COMMAND ${CMAKE_COMMAND} -E echo "  cd build-msan"
        COMMAND ${CMAKE_COMMAND} -E echo "  ./splat_tests"
        COMMENT "MSan information - not running, see output"
    )
else()
    add_custom_target(msan
        COMMAND ${CMAKE_COMMAND} -E echo "MemorySanitizer requires Clang compiler"
        COMMAND ${CMAKE_COMMAND} -E echo "Current compiler: ${CMAKE_CXX_COMPILER_ID}"
        COMMAND ${CMAKE_COMMAND} -E echo "To use MSan, reconfigure with: CC=clang CXX=clang++ cmake .."
        COMMAND ${CMAKE_COMMAND} -E false
        COMMENT "MSan not available with current compiler"
    )
endif()

# LeakSanitizer - detects memory leaks (can be run standalone or with ASan)
add_custom_target(lsan
    COMMAND ${CMAKE_COMMAND} -E echo "Running LeakSanitizer..."
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/lsan
    COMMAND ${CMAKE_COMMAND}
        "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}"
        "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
        "-DCMAKE_CXX_FLAGS=-fsanitize=leak -fno-omit-frame-pointer -g -O1"
        "-DCMAKE_C_FLAGS=-fsanitize=leak -fno-omit-frame-pointer -g -O1"
        "-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=leak"
        -S ${CMAKE_SOURCE_DIR}
        -B ${CMAKE_BINARY_DIR}/lsan
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}/lsan --target splat_tests
    COMMAND ${CMAKE_COMMAND} -E echo "Running tests with LeakSanitizer..."
    COMMAND cd ${CMAKE_BINARY_DIR}/lsan && ASAN_OPTIONS=detect_leaks=1 ./splat_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running LeakSanitizer"
    VERBATIM
)

# Valgrind - memory error detector and profiler
add_custom_target(valgrind
    COMMAND ${CMAKE_COMMAND} -E echo "Running Valgrind..."
    COMMAND valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes
        --verbose --error-exitcode=1 ${CMAKE_BINARY_DIR}/splat_tests
    DEPENDS splat_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running Valgrind memory checker"
    VERBATIM
)

# Valgrind with memcheck only (faster)
add_custom_target(valgrind-quick
    COMMAND ${CMAKE_COMMAND} -E echo "Running Valgrind (quick)..."
    COMMAND valgrind --leak-check=yes --error-exitcode=1 ${CMAKE_BINARY_DIR}/splat_tests
    DEPENDS splat_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running Valgrind (quick mode)"
    VERBATIM
)

# Clang-Tidy - static analysis
add_custom_target(clang-tidy
    COMMAND ${CMAKE_COMMAND} -E echo "Running Clang-Tidy..."
    COMMAND clang-tidy
        ${SPLAT_SOURCES}
        -p ${CMAKE_BINARY_DIR}
        --checks='-*,bugprone-*,clang-analyzer-*,cppcoreguidelines-*,modernize-*,performance-*,readability-*'
        --warnings-as-errors=''
        --header-filter='.*'
    DEPENDS splat
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running Clang-Tidy static analysis"
    VERBATIM
)

# CppCheck - static analysis
add_custom_target(cppcheck
    COMMAND ${CMAKE_COMMAND} -E echo "Running CppCheck..."
    COMMAND cppcheck
        --enable=all
        --inconclusive
        --std=c++17
        --suppress=missingIncludeSystem
        --suppress=unmatchedSuppression
        --inline-suppr
        --error-exitcode=1
        --quiet
        -I src
        src/
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running CppCheck static analysis"
    VERBATIM
)

# CppCheck with verbose output
add_custom_target(cppcheck-verbose
    COMMAND ${CMAKE_COMMAND} -E echo "Running CppCheck (verbose)..."
    COMMAND cppcheck
        --enable=all
        --inconclusive
        --std=c++17
        --suppress=missingIncludeSystem
        -I src
        src/
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running CppCheck static analysis (verbose)"
    VERBATIM
)

# Combined sanitizers target (runs all sanitizers sequentially)
# Note: MSan only runs on Clang
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_custom_target(sanitizers
        COMMAND ${CMAKE_COMMAND} -E echo "Running all sanitizers..."
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target asan
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target ubsan
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target tsan
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target lsan
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target msan
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running all sanitizers sequentially (including MSan)"
        VERBATIM
    )
else()
    add_custom_target(sanitizers
        COMMAND ${CMAKE_COMMAND} -E echo "Running all sanitizers (MSan skipped - requires Clang)..."
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target asan
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target ubsan
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target tsan
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target lsan
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running all sanitizers sequentially (MSan not available)"
        VERBATIM
    )
endif()

# Combined static analysis target
add_custom_target(analyze
    COMMAND ${CMAKE_COMMAND} -E echo "Running all static analysis tools..."
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target cppcheck
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target clang-tidy
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running all static analysis tools"
    VERBATIM
)

# All checks target (sanitizers + static analysis + valgrind)
add_custom_target(check-all
    COMMAND ${CMAKE_COMMAND} -E echo "Running comprehensive checks..."
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target sanitizers
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target analyze
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target valgrind
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running all checks (sanitizers, static analysis, valgrind)"
    VERBATIM
)
