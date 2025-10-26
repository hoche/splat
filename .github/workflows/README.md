# GitHub Actions Workflows

This directory contains GitHub Actions workflows for continuous integration, testing, and code quality analysis.

## Workflows

### 1. CI Build and Test (`ci.yml`)

**Purpose**: Build and test SPLAT! on all supported platforms.

**Triggers**:
- Push to main/master/develop/v2.0.0-b1 branches
- Pull requests to main/master/develop/v2.0.0-b1 branches
- Manual workflow dispatch

**Platforms Tested**:
- **Linux**: Ubuntu (latest) with GCC and Clang, Debug and Release builds
- **macOS Intel**: macOS 13 on x86_64, Debug and Release builds
- **macOS ARM**: macOS 14 on Apple Silicon (M1+), Debug and Release builds
- **Windows**: Windows (latest) with MSVC, Debug and Release builds
- **FreeBSD**: FreeBSD 14.0, Debug and Release builds

**Build Matrix**: 10 total configurations
- Linux: 4 builds (2 compilers × 2 build types)
- macOS Intel: 2 builds
- macOS ARM: 2 builds
- Windows: 2 builds
- FreeBSD: 2 builds

**Features**:
- Parallel builds for faster CI
- Test result uploads as artifacts
- Build summary job to check overall status
- Uses Ninja for faster builds where possible

### 2. Code Coverage (`coverage.yml`)

**Purpose**: Generate code coverage reports for unit tests.

**Triggers**:
- Push to main/master/develop/v2.0.0-b1 branches
- Pull requests to main/master/develop/v2.0.0-b1 branches
- Manual workflow dispatch

**Tools Used**:
- **lcov**: Coverage data collection
- **gcovr**: Alternative coverage reporting
- **Codecov**: Online coverage dashboard
- **genhtml**: HTML coverage report generation

**Outputs**:
- Coverage percentage badge
- Codecov integration
- HTML coverage report (artifact)
- Console coverage summary

### 3. Static Analysis and Sanitizers (`analysis.yml`)

**Purpose**: Detect bugs, memory issues, and code quality problems.

**Jobs**:

#### AddressSanitizer (ASan)
- Detects memory errors (buffer overflows, use-after-free, memory leaks)
- Uses Clang with `-fsanitize=address`

#### UndefinedBehaviorSanitizer (UBSan)
- Detects undefined behavior (integer overflow, null pointer dereference)
- Uses Clang with `-fsanitize=undefined`

#### ThreadSanitizer (TSan)
- Detects data races and threading issues
- Uses Clang with `-fsanitize=thread`

#### Clang-Tidy
- Static analysis for C++ code
- Checks for bugs, performance issues, style violations
- Generates compile_commands.json for analysis

#### Valgrind
- Memory leak detection
- Invalid memory access detection
- Uninitialized memory usage detection

#### Cppcheck
- Additional static analysis
- Checks for common C/C++ programming errors
- Generates XML report (artifact)

## Local Development

### Running Tests Locally

```bash
# Configure and build
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Run tests
cd build
./splat_tests
```

### Running with Sanitizers

```bash
# AddressSanitizer
cmake -B build-asan \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer -g" \
  -DCMAKE_C_FLAGS="-fsanitize=address -fno-omit-frame-pointer -g"
cmake --build build-asan
cd build-asan
./splat_tests

# UndefinedBehaviorSanitizer
cmake -B build-ubsan \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=undefined -g" \
  -DCMAKE_C_FLAGS="-fsanitize=undefined -g"
cmake --build build-ubsan
cd build-ubsan
./splat_tests
```

### Running Coverage Locally

```bash
# Configure with coverage flags
cmake -B build-coverage \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="--coverage" \
  -DCMAKE_C_FLAGS="--coverage"

# Build and test
cmake --build build-coverage
cd build-coverage
./splat_tests

# Generate coverage report
lcov --capture --directory . --output-file coverage.info
lcov --remove coverage.info '/usr/*' '*/tests/*' '*/_deps/*' --output-file coverage.info
genhtml coverage.info --output-directory coverage_html

# View in browser
xdg-open coverage_html/index.html
```

### Running Static Analysis Locally

```bash
# Clang-Tidy
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
clang-tidy -p build src/*.cpp

# Cppcheck
cppcheck --enable=all --std=c++17 src/

# Valgrind
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
valgrind --leak-check=full ./build/splat_tests
```

## Requirements

### Dependencies
- CMake 3.16 or higher
- C++17 compatible compiler
- libbz2-dev (bzip2 development headers)
- zlib1g-dev (zlib development headers)
- GoogleTest (fetched automatically by CMake)

### Platform-Specific Notes

#### Linux
```bash
sudo apt-get install cmake ninja-build libbz2-dev zlib1g-dev
```

#### macOS
```bash
brew install cmake ninja bzip2
```

#### Windows
```powershell
choco install cmake ninja
```

#### FreeBSD
```bash
sudo pkg install cmake ninja bzip2 googletest
```

## Troubleshooting

### Test Failures
- Check the uploaded test result artifacts
- Review the test output in the workflow logs
- Run tests locally to reproduce

### Build Failures
- Check compiler version compatibility
- Ensure all dependencies are installed
- Review build logs for specific errors

### Coverage Issues
- Ensure tests are actually running
- Check that coverage flags are applied to all source files
- Verify lcov/gcov is installed and working

### Sanitizer Failures
- Review sanitizer output for specific issues
- Memory leaks may be in dependencies
- Some false positives may occur

## Contributing

When submitting pull requests:
1. Ensure all CI checks pass
2. Add tests for new functionality
3. Maintain or improve code coverage
4. Fix any sanitizer warnings
5. Address static analysis issues

## Support

For issues with CI/CD:
- Check GitHub Actions logs
- Review workflow YAML syntax
- Consult GitHub Actions documentation
- Open an issue with relevant logs

## License

These workflows are part of the SPLAT! project and follow the same license.
