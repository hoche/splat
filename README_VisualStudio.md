# SPLAT! for Windows (command line)

SPLAT! builds as a Win32/Win64 console application via CMake. There is no GUI.

### NOTE: SDF files under Windows *must* use `_` instead of `:` in the name

For example, a file formerly named `44:45:120:121.sdf` must be `44_45_120_121.sdf`.
Unix and macOS accept either form.

## Requirements

- Visual Studio 2019 or newer (Community Edition is fine), with the
  "Desktop development with C++" workload, **or** Build Tools for Visual Studio
- CMake 3.16 or newer
- A C++17 compiler (MSVC from the above Visual Studio install)
- Dependencies via [vcpkg](https://vcpkg.io/) (recommended): bzip2, zlib, libpng;
  optionally libjpeg-turbo, gdal, curl, and libwebp

## Quick start (vcpkg + CMake)

1. Clone the repository and bootstrap vcpkg (once):

   ```powershell
   git clone https://github.com/Microsoft/vcpkg.git
   .\vcpkg\bootstrap-vcpkg.bat
   .\vcpkg\vcpkg install bzip2:x64-windows zlib:x64-windows libpng:x64-windows `
     libjpeg-turbo:x64-windows gdal:x64-windows curl:x64-windows libwebp:x64-windows
   ```

2. Configure and build SPLAT!:

   ```powershell
   git clone https://github.com/Supermagnum/splat.git
   cd splat
   cmake -B build -DCMAKE_TOOLCHAIN_FILE="..\vcpkg\scripts\buildsystems\vcpkg.cmake" `
     -DCMAKE_BUILD_TYPE=Release
   cmake --build build --config Release -j
   ```

3. Run `splat.exe` with the same command-line options as on Unix. With the default
   Visual Studio generator the binary is usually under `build\Release\`.

Point `CMAKE_TOOLCHAIN_FILE` at your vcpkg clone if it is not next to the `splat`
directory. Use an `arm64-windows` (or other) triplet if that matches your machine.

## Visual Studio (Open Folder / CMake)

Visual Studio 2019 and later support CMake projects without a hand-written `.sln`:

1. Clone the repository and install vcpkg packages as above.
2. File -> Open -> Folder and open the root `splat` directory.
3. Configure CMake so the toolchain file is set, for example in
   `CMakeSettings.json` / CMakePresets, or via
   `CMAKE_TOOLCHAIN_FILE` pointing at
   `...\vcpkg\scripts\buildsystems\vcpkg.cmake`.
4. Build from the CMake menu (or right-click the root `CMakeLists.txt`).
5. Run `splat.exe` with the same options you would use on Unix.

## Optional tools

- **gnuplot** — needed for terrain and elevation profile plots. Install from
  [SourceForge](https://sourceforge.net/projects/gnuplot/) or with Chocolatey:
  `choco install gnuplot`.

## Troubleshooting

- CMake cannot find zlib/bzip2/png: ensure `CMAKE_TOOLCHAIN_FILE` points at
  vcpkg's `vcpkg.cmake`, and that you installed packages for the same triplet
  (for example `x64-windows`) that CMake is targeting.
- Missing PNG symbols at link time: rebuild dependencies and SPLAT! for the same
  architecture (x64 vs ARM64).
- For the short Linux/macOS-oriented overview, see the
  [Building](README.md#building) section in `README.md`.
