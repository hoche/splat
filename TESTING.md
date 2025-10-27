# Testing and Code Quality Tools

This document describes the various testing, sanitizer, and static analysis tools available in the SPLAT! build system.

## Quick Start

```bash
# Build and run basic tests
make test

# Quick check with common sanitizers
make check-quick

# Run all checks (takes longer)
make check-all
```

## Available Tools

### Basic Testing

```bash
make test           # Run all unit tests
```

### Sanitizers (Runtime Error Detection)

Sanitizers instrument your code at compile-time to detect various runtime errors:

#### AddressSanitizer (ASan)
Detects memory errors including:
- Buffer overflows and underflows
- Use-after-free
- Use-after-return
- Use-after-scope
- Double-free
- Memory leaks (when combined with LeakSanitizer)

```bash
make asan
```

#### UndefinedBehaviorSanitizer (UBSan)
Detects undefined behavior including:
- Signed integer overflow
- Unaligned memory access
- Null pointer dereference
- Division by zero
- Array bounds violations

```bash
make ubsan
```

#### ThreadSanitizer (TSan)
Detects concurrency issues:
- Data races
- Deadlocks
- Lock order inversions

```bash
make tsan
```

#### LeakSanitizer (LSan)
Detects memory leaks:
- Memory that was allocated but never freed

```bash
make lsan
```

#### MemorySanitizer (MSan)
Detects uninitialized memory reads (Clang only):
- Reads from uninitialized variables
- Use of uninitialized memory

```bash
make msan
```

**Important Notes**:
- MSan **requires Clang** compiler (not available with GCC)
- MSan requires all dependencies to be compiled with MSan, which can be challenging
- **Good news**: SPLAT! now defaults to Clang if available, so MSan should work out of the box!
- If you explicitly configured with GCC, the `make msan` target will display a helpful error message

**Common Issue - False Positives from System Libraries**:

MSan will report uninitialized values in system libraries (libproj, libgdal, etc.) because they weren't compiled with MSan. These are false positives and can be safely ignored if they're in third-party code:

```
WARNING: MemorySanitizer: use-of-uninitialized-value
  #0 ... in libproj.so.25
  #1 ... in libgdal.so.34
```

To focus on SPLAT! code only, look for stack traces that include `/home/.../splat/src/` paths.

To ensure Clang is being used:
```bash
# CMake automatically prefers Clang if available
cmake -B build

# Or explicitly use Clang
CC=clang CXX=clang++ cmake -B build
make msan
```

#### Run All Sanitizers

```bash
make sanitizers     # Runs ASan, UBSan, TSan, LSan (+ MSan if using Clang)
```

**Note**: The `sanitizers` target automatically skips MSan if using GCC.

### Memory Analysis

#### Valgrind
Comprehensive memory debugging and profiling:
- Memory leak detection
- Use of uninitialized memory
- Invalid memory access
- Memory corruption

```bash
make valgrind         # Full analysis (slower, very thorough)
make valgrind-quick   # Quick analysis (faster, less detail)
```

**Note**: Valgrind is slower than sanitizers but doesn't require recompilation.

### Static Analysis

Static analysis tools examine your code without running it:

#### Clang-Tidy
Modern C++ linter and static analyzer:
- Finds bugs and code smells
- Suggests modernizations
- Checks coding guidelines
- Analyzes performance issues

```bash
make clang-tidy
```

Checks enabled:
- `bugprone-*` - Bug-prone code patterns
- `clang-analyzer-*` - Clang static analyzer
- `cppcoreguidelines-*` - C++ Core Guidelines
- `modernize-*` - Modern C++ suggestions
- `performance-*` - Performance issues
- `readability-*` - Readability improvements

#### CppCheck
C/C++ static analysis tool:
- Finds bugs and undefined behavior
- Checks for memory leaks
- Detects unused functions and variables
- Identifies potential security issues

```bash
make cppcheck          # Quiet mode (errors only)
make cppcheck-verbose  # Verbose mode (all findings)
```

#### Run All Static Analysis

```bash
make analyze    # Runs both Clang-Tidy and CppCheck
```

## Combined Targets

### Quick Check
Runs the fastest and most common sanitizers (ASan + UBSan):

```bash
make check-quick
```

Recommended for:
- Local development
- Pre-commit checks
- Quick validation

### Comprehensive Check
Runs all available tools:

```bash
make check-all
```

Includes:
1. All sanitizers (ASan, UBSan, TSan, LSan)
2. All static analysis (Clang-Tidy, CppCheck)
3. Valgrind memory analysis

Recommended for:
- CI/CD pipelines
- Pre-release validation
- Thorough code review

## Interpreting Results

### Sanitizer Output

Sanitizers print detailed reports when they detect issues:

```
=================================================================
==12345==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x...
...
SUMMARY: AddressSanitizer: heap-buffer-overflow ...
```

Key information:
- Error type (e.g., heap-buffer-overflow)
- Stack trace showing where the error occurred
- Address and size information
- Shadow bytes (for ASan)

### Exit Codes

All tools return non-zero exit codes on failure:
- `0` = Success (no issues found)
- `1` = Failure (issues found)

This makes them suitable for CI/CD integration:

```bash
make asan || exit 1
```

## Continuous Integration

Example GitHub Actions workflow:

```yaml
- name: Run Sanitizers
  run: make sanitizers

- name: Run Static Analysis
  run: make analyze

- name: Run Valgrind
  run: make valgrind-quick
```

## Performance Notes

Approximate runtime overhead:

| Tool | Slowdown | Memory Overhead | Notes |
|------|----------|-----------------|-------|
| ASan | 2x | 3x | Best all-around memory checker |
| UBSan | ~20% | Minimal | Very fast, highly recommended |
| TSan | 5-15x | 5-10x | Only for multi-threaded code |
| MSan | 3x | Large | Requires special build of dependencies |
| LSan | Minimal | Minimal | Can be combined with ASan |
| Valgrind | 10-50x | Large | No recompilation needed |

## Best Practices

1. **Development workflow**:
   ```bash
   # Regular development
   make test

   # Before committing
   make check-quick

   # Before major releases
   make check-all
   ```

2. **Fix issues in order**:
   - Start with ASan (memory errors are often root causes)
   - Then UBSan (undefined behavior can cause crashes)
   - Then TSan (if you have threading code)
   - Finally static analysis (code quality)

3. **CI/CD Integration**:
   - Run `make check-quick` on every commit
   - Run `make check-all` nightly or on release branches
   - Cache sanitizer builds for faster CI

4. **Debugging failures**:
   - Sanitizers set breakpoints: `ASAN_OPTIONS=abort_on_error=1`
   - Get more detail: `ASAN_OPTIONS=verbosity=1`
   - Disable leak detection temporarily: `ASAN_OPTIONS=detect_leaks=0`

## Troubleshooting

### "sanitizer not found"
Install compiler with sanitizer support:
```bash
# Ubuntu/Debian
sudo apt-get install clang

# The sanitizers are built into modern GCC/Clang
```

### "valgrind: command not found"
```bash
sudo apt-get install valgrind
```

### "clang-tidy: command not found"
```bash
sudo apt-get install clang-tidy
```

### "cppcheck: command not found"
```bash
sudo apt-get install cppcheck
```

### False Positives

Suppress false positives:

**ASan/LSan**:
Create a suppression file and set:
```bash
export LSAN_OPTIONS=suppressions=lsan.supp
```

**Valgrind**:
```bash
valgrind --suppressions=valgrind.supp ./splat_tests
```

**CppCheck**:
Use inline suppressions in code:
```cpp
// cppcheck-suppress memleak
void* ptr = malloc(10);
```

## Further Reading

- [AddressSanitizer Wiki](https://github.com/google/sanitizers/wiki/AddressSanitizer)
- [UndefinedBehaviorSanitizer](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html)
- [ThreadSanitizer](https://github.com/google/sanitizers/wiki/ThreadSanitizerCppManual)
- [Valgrind Manual](https://valgrind.org/docs/manual/manual.html)
- [Clang-Tidy Documentation](https://clang.llvm.org/extra/clang-tidy/)
- [CppCheck Manual](https://cppcheck.sourceforge.io/manual.pdf)
