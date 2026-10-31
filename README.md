# SPLAT!

A Terrestrial RF Path and Terrain Analysis Tool for Unix/Linux

## Table of contents

- [About](#about)
- [Dependencies / packages](#dependencies--packages)
- [Building](#building)
- [Installation](#installation)
- [Running](#running)
- [Mapterhorn](#mapterhorn-terrain-optional)
- [splat-batch](#splat-batch)
- [Testing / test results](#testing-and-code-quality)
- [LA5MR batch results](docs/la5mr_batch_results.md)
- [test-results/](test-results/) (checked-in LA5MR batch outputs)
- [Changes](#changes)
- [To Do](#to-do)
- [Note about lrp files](#note-about-lrp-files)
- [Acknowledgements](#acknowledgements)

## About

This version is a refactoring of the code in 1.5, along with some enhancements to support GDAL.
It otherwise does exactly the same calculations, and has the bugfixed and enhancements of 1.5,
including multithreading and GDAL support.

The "classic" SPLAT version 1.5 is also available as a separate branch. It is based off John
Magliacane's 1.4.2 release, but with a few bugfixes.
However, we recommend this 2.0 branch for both stability and performance.

Future version may either use OpenCL or Vulkan to hand computation off to a graphics
card in the hopes of even more speed improvements. In preparation for this, itwom3.0 was
made fully C99-compliant, as all the current implementations of OpenCL drivers require
that. (Later versions of OpenCL allow C++, but none of the common GPU drivers support that).

Optional **Mapterhorn** terrain support builds SDF pages on demand from PMTiles WebP
elevation tiles (`-mapterhorn`). The **splat-batch** Python driver runs many per-repeater
coverage jobs and writes GeoJSON / FlatGeobuf polygons. See the sections below and
[docs/test_results.md](docs/test_results.md) for verified acceptance results.

## Dependencies / packages

### Required for SPLAT!

- CMake (3.16 or newer)
- A C++17 compiler (gcc or clang)
- zlib
- libbz2 (bzip2)

### Optional image libraries

SPLAT! builds without these, but output will be large, low-quality `.ppm` files unless
PNG/JPEG (and optionally GDAL) are present:

- libpng
- libjpeg
- libgdal

### Optional: Mapterhorn (`-mapterhorn`, `mapterhorn2sdf`)

- libcurl
- libwebp

Without libcurl and libwebp, SPLAT! still builds normally, but the `-mapterhorn`
options and the `mapterhorn2sdf` helper are disabled.

### Optional: splat-batch

- Python 3
- numpy
- shapely
- GDAL CLI tools: `gdal_translate`, `gdal_polygonize` (or `gdal_polygonize.py`), `ogr2ogr`

### Graphs

- gnuplot (for path-profile graphs)

### Centos 7

```
yum install cmake gcc-c++ bzip2-devel zlib-devel libpng-devel libjpeg-turbo-devel \
  gdal-devel libcurl-devel libwebp-devel gnuplot \
  python3 python3-numpy python3-shapely gdal
```

Package names for Python/GDAL CLI vary by CentOS/RHEL release; adjust if needed.

### Debian and Ubuntu

Supported examples:

- Debian Trixie and Ubuntu 24.04 LTS
- Debian Bookworm and Ubuntu 22.04 LTS
- Debian Bullseye and Ubuntu 20.04 LTS
- Debian Buster and Ubuntu 18.04 LTS

```
apt-get install cmake g++ clang libbz2-dev zlib1g-dev \
  libjpeg-dev libpng-dev libgdal-dev \
  libcurl4-openssl-dev libwebp-dev gnuplot \
  python3 python3-numpy python3-shapely gdal-bin
```

### macOS / Homebrew

Works on both Intel and Apple Silicon (ARM). Install [Homebrew](https://brew.sh/), then:

```
brew install cmake ninja jpeg libpng libgdal curl webp bzip2 zlib gnuplot python numpy
```

Install shapely (`pip3 install shapely` or `brew install shapely`) and ensure GDAL CLI
tools are on `PATH` (Homebrew `gdal` provides them).

### Windows

See [Building on Windows](#building-on-windows) below and
[README_VisualStudio.md](README_VisualStudio.md) for Visual Studio details.

## Building

You must have CMake and a C++17 compiler (gcc, clang, or MSVC on Windows).

**Note**: On Unix-like systems the build prefers Clang if available (for better sanitizer
support), but falls back to GCC if Clang is not found. You can override this by setting
`CC` and `CXX` environment variables:

```bash
# Use GCC explicitly
CC=gcc CXX=g++ cmake -B build

# Use Clang explicitly
CC=clang CXX=clang++ cmake -B build

# Let CMake choose (prefers Clang)
cmake -B build
```

### Compile on Linux (CMake)

From the repository root:

```bash
git clone https://github.com/Supermagnum/splat.git
cd splat
mkdir build
cd build
cmake ..
cmake --build . -j
```

Equivalently, from the repository root with the top-level Makefile (creates `build/`,
runs `cmake --fresh`, then `cmake --build`):

```bash
make
```

CMake reports whether Mapterhorn support is enabled. If libcurl or libwebp is missing,
you will see `Mapterhorn support disabled`; `splat` and the other utilities still build.

### Example build on Ubuntu 24.04 LTS

After installing packages as indicated above:

```
git clone https://github.com/Supermagnum/splat.git
mkdir splat/build
cd splat/build
cmake ..
make
```

### Building on macOS

After installing Homebrew packages as indicated above:

```bash
git clone https://github.com/Supermagnum/splat.git
cd splat
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

On Apple Silicon, if CMake cannot find Homebrew libraries under `/opt/homebrew`, pass the
prefix explicitly:

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$(brew --prefix)"
cmake --build build -j
```

The `splat` binary is then at `build/splat`. Install with:

```bash
cmake --install build
```

### Building on Windows

SPLAT! is a console application. Use Visual Studio 2019 or newer (Community Edition is
fine), or another C++17 toolchain, with [CMake](https://cmake.org/) and
[vcpkg](https://vcpkg.io/) for dependencies.

**Note:** On Windows, SDF filenames must use `_` instead of `:` (for example
`44_45_120_121.sdf` instead of `44:45:120:121.sdf`). Unix and macOS accept either form.

#### 1. Install tools

- Visual Studio 2019+ with the "Desktop development with C++" workload, **or**
  Build Tools for Visual Studio
- [Git](https://git-scm.com/)
- CMake (or `choco install cmake ninja`)

#### 2. Install libraries with vcpkg

From a Developer PowerShell or Command Prompt:

```powershell
git clone https://github.com/Microsoft/vcpkg.git
.\vcpkg\bootstrap-vcpkg.bat
.\vcpkg\vcpkg install bzip2:x64-windows zlib:x64-windows libpng:x64-windows `
  libjpeg-turbo:x64-windows gdal:x64-windows curl:x64-windows libwebp:x64-windows
```

Adjust the triplet (`x64-windows`, `arm64-windows`, …) to match your platform.

#### 3. Configure and build

```powershell
git clone https://github.com/Supermagnum/splat.git
cd splat
cmake -B build -DCMAKE_TOOLCHAIN_FILE="..\vcpkg\scripts\buildsystems\vcpkg.cmake" `
  -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j
```

If `vcpkg` lives elsewhere, set `CMAKE_TOOLCHAIN_FILE` to that clone's
`scripts\buildsystems\vcpkg.cmake`. The executable is typically
`build\Release\splat.exe` (or `build\splat.exe` with Ninja).

#### 4. Optional: Visual Studio GUI

Open the repository folder in Visual Studio (File -> Open -> Folder), ensure the CMake
settings use the vcpkg toolchain file, then build from the CMake menu. See
[README_VisualStudio.md](README_VisualStudio.md) for more detail.

For path-profile graphs, install [gnuplot](https://sourceforge.net/projects/gnuplot/)
or `choco install gnuplot`.

## Installation

From the `build` directory after a successful compile:

```bash
cmake --install .
# or:
make install
```

Default install layout (prefix is usually `/usr/local`):

| Target | When installed | Destination |
|--------|----------------|-------------|
| `splat` | always | `bin/` |
| `mapterhorn2sdf` | only if libcurl and libwebp were found at configure time | `bin/` |
| `splat-batch` | always (Python launcher) | `bin/` |
| `splat_batch` package | always | `share/splat/splat_batch/` |
| other utils (`srtm2sdf`, `bearing`, ...) | always | `bin/` |

Use `cmake --install . --prefix /path/to/prefix` to choose a non-default prefix.

## Running

Topography data must be downloaded and SPLAT Data Files must
be generated using the included `srtm2sdf`, `postdownload`, or `usgs2sdf`
utilities before using SPLAT!  Instructions for doing so are included
in the documentation. Alternatively, use [Mapterhorn](#mapterhorn-terrain-optional)
to generate SDF pages on demand.

It is a good practice to create a working directory for SPLAT! use
under your home directory:

    mkdir $HOME/splat-work

Then:

    cd $HOME/splat-work

before invoking SPLAT!

In this manner, all associated SPLAT! working files can be kept in a
common directory.

It is important to realize that when analyzing regional coverage
areas of transmitters, repeaters, or cell sites, SPLAT! Data Files
need to be available for the entire region surrounding the site(s)
being analyzed.  SPLAT! Data Files can be placed under your SPLAT!
working directory, or under a separate directory specified in your
`$HOME/.splat_path` file so SPLAT! can easily find them.

### Mapterhorn terrain (optional)

Instead of generating SDF files by hand, SPLAT! can build them on demand from
[Mapterhorn](https://mapterhorn.com) terrain data stored in a PMTiles archive.
Add `-mapterhorn` to the command line; the needed 1x1 degree SDF pages are
generated by the `mapterhorn2sdf` helper into the directory given by `-mt-cache`
(default: the `-d` directory, else `./sdf-cache`):

    splat -hd -t site.qth -L 1.5 -R 100 -mapterhorn \
          -mt-source /data/mapterhorn/norway.pmtiles -mt-cache /data/sdf -o site

This requires libcurl and libwebp at build time. For production use, point
`-mt-source` at a local PMTiles extract rather than the public server. See the
Mapterhorn section of [docs/manual/english/splat.md](docs/manual/english/splat.md),
the `mapterhorn2sdf` and `splat-batch` sections of [utils/README.md](utils/README.md),
and [docs/data_file_formats.md](docs/data_file_formats.md) for details.

Terrain data: Mapterhorn. Attribution is required: https://mapterhorn.com/attribution

### splat-batch

`splat-batch` (installed from `utils/splat-batch`, package under `utils/splat_batch`)
runs many independent SPLAT! coverage jobs from a repeater list (`.joz`, GeoJSON, or
CSV), optionally prefetches Mapterhorn terrain, polygonizes hear/talk areas, and writes
per-county GeoJSON and FlatGeobuf. Example:

    splat-batch --input repeaters.joz --out-dir ./out \
      --mapterhorn --mt-source /data/norway.pmtiles --mt-cache /data/sdf \
      --splat-bin ./build/splat --hd --workers 4

See [utils/README.md](utils/README.md) and [utils/splat_batch/README.md](utils/splat_batch/README.md)
for inputs, caching, ITM error handling, and options. Python/numpy/shapely and the GDAL
CLI tools listed under [Dependencies](#dependencies--packages) are required.

Please read the README file under the utils directory for information
on the utilities included with SPLAT!.

Please read the documentation under `docs` directory,
or consult the program's man page for more information and examples
of SPLAT! use.

## Changes

* Build system

The build system has been converted to CMake.
  
* SDF file names

  In order to accommodate Windows filesystems, the basic format of the SDF file naming has changed.
  The colon has been changed to an underline. For instance, instead of having a file named `46:47:122:123.sdf`
  it should now be `46_47_122_123.sdf`. The various unix versions (including OSX) will handle either version,
  but by default srtm2sdf will now create names with the underline. If you don't want this, a "-c" flag has
  been added to srtm2sdf to preserve your colon.

* splat.cpp

  * Incorporate John's antenna height changes from SPLAT 1.4.3 (unreleased).
  
  * Revert to using the ITM model by default, in accordance with SPLAT 1.4.3.
  
  * The PlotLOSMap() and PlotLRMap() functions have been converted to run multithreaded ~~if a "-mt" flag is
    passed on the command line~~. If you want to run single-threaded, use "-st" on the command line.

  * WritePPM(), WritePPMSS(), etc were converted to WriteImage(), WriteImageSS(), etc, and functionality
    was added to allow them to emit png or jpg images instead of pixmaps. png's are now the default. Add "-ppm"
    or "-jpg" to the command line if you want to generate the others. The generated jpg's are smaller but the text
    can be hard to read in some instances. The png's are, of course, lossless, and nice and crisp, but they are
    larger and take slightly longer to generate. Both are an order of magnitude smaller than the pixmaps though.
    
  * All the DEMs and Paths are allocated on the heap rather than using pre-sized arrays and stack allocations.
    This means the same code can be used for 1x1 through 8x8 grids, and/or 3-deg (Normal) or 1-deg (High Definition)
    modes without recompiling. It also means that the code can adjust itself (or error out nicely) depending on the
    available memory of the host machine.
    
  * I did some minor fixup to the antenna azimuth (.az) and elevation (.el) reading so that you can now put a value
    of 0 in those files to indicate your antenna blocks that area perfectly.
    
* ITWOM 3.0

  * itwom3.0.cpp was renamed to itwom3.0.c and is now compiled with the C compiler rather than the C++ one.
    In addition, every effort was made to make it fully C99-compliant rather than the peculiar amalgam of C
    and C++ syntax that it was using.
  
    itwom3.0 no longer uses the C++ complex number templates (obtained by doing #include <complex>). Instead
    a small complex-number library is introduced. This was done as part of the effort to make it fully
    C99 compliant.

  * All the static variables were removed from function scopes within itwom3.0.c in an effort to make the code
    fully reentrant. This had resulted in a number of changes:
        - There are a few cases where these were actually consts; those have been defined as such
        and moved to the global scope.
        - In a few cases this means that we recalculate variables multiple times.
        - In other cases, new "state" structs have been introduced that act as local contexts for repeated
        calls to the same function.
    All these changes are geared towards making the code multithread-safe.
    
  * A few variables that are not modified by the various functions have been declared as const. This includes
    pfl[] arrays.

  * A number of minor fixes were made to itwom3.0 that were disguised by using the C++ compiler. For instance,
    in a number of places abs() was called when fabs() was meant.
    
  * Much much code documenting was done. There remains a lot to do though.

* Mapterhorn and splat-batch

  * Optional `-mapterhorn` / `-mt-*` flags generate SDF pages on demand via `mapterhorn2sdf`
    (requires libcurl and libwebp).
  * `splat-batch` batch-runs coverage jobs and writes per-county polygons.

## To Do

* Since we have to link to zlib for the pngs, we might as well create kmz files if asked.
* More code cleanup.
* Reformat tabs to four spaces.

## Note about lrp files

vim will try to interpret these as "Linux Router Project" files. You can disablethis by setting:

	let g:loaded_tarPlugin = 1
	let g:loaded_tar = 1

in your ~/.vimrc.

## Testing and Code Quality

SPLAT! includes comprehensive testing and code quality tools.

Verified acceptance results for Mapterhorn and splat-batch (unit-test counts, terrain
checks, path-loss comparison, batch dry-run, despike, ITM error handling, and related
notes) are in **[docs/test_results.md](docs/test_results.md)**.

A limited LA5MR-network-only splat-batch run (11 jobs; full Norway not run) is
documented in **[docs/la5mr_batch_results.md](docs/la5mr_batch_results.md)**.
County GeoJSON/FlatGeobuf, Google Earth KML/KMZ, coverage-map and gnuplot
path-profile samples from that run are in **[test-results/](test-results/)**.

### Running Tests

```bash
make test           # Run all unit tests
```

### Sanitizers (Runtime Error Detection)

Detect memory errors, undefined behavior, and concurrency issues:

```bash
make asan           # AddressSanitizer (memory errors)
make ubsan          # UndefinedBehaviorSanitizer (undefined behavior)
make tsan           # ThreadSanitizer (data races, deadlocks)
make lsan           # LeakSanitizer (memory leaks)
make sanitizers     # Run all sanitizers
```

### Static Analysis

Analyze code without running it:

```bash
make cppcheck       # CppCheck static analysis
make clang-tidy     # Clang-Tidy analysis
make analyze        # Run all static analysis tools
```

### Memory Analysis

```bash
make valgrind       # Valgrind comprehensive analysis
make valgrind-quick # Valgrind quick mode
```

### Combined Checks

```bash
make check-quick    # Quick check (ASan + UBSan)
make check-all      # Run all checks (comprehensive)
```

For detailed information about each tool, see [TESTING.md](TESTING.md).

## Acknowledgements

This project and code is based on the original SPLAT! version 1.4.2 by John A. Magliacane, KD2BD:
http://www.qsl.net/kd2bd/splat.html
