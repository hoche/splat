# SPLAT! Documentation

Welcome to the SPLAT! documentation directory. This directory contains comprehensive documentation for the SPLAT! (Signal Propagation, Loss, And Terrain analysis) tool.

## Directory Structure

```
docs/
├── README.md                 # This file
├── architecture.md           # System architecture and component overview
├── data_file_formats.md      # Complete guide to all SPLAT! data file formats
├── CMakeLists.txt           # CMake build configuration for documentation
└── manual/                  # Manual pages in multiple languages
    ├── english/
    │   ├── splat.man        # English manual page (troff format)
    │   ├── splat.md         # English manual page (Markdown format)
    │   └── CMakeLists.txt   # Build configuration for English manual
    ├── spanish/
    │   ├── splat.man        # Spanish manual page (troff format)
    │   └── CMakeLists.txt   # Build configuration for Spanish manual
    ├── french/
    │   ├── splat.man        # French manual page (troff format)
    │   └── CMakeLists.txt   # Build configuration for French manual
    └── german/
        ├── splat.man        # German manual page (troff format)
        └── CMakeLists.txt   # Build configuration for German manual
```

## Documentation Files

### Core Documentation

- **[architecture.md](architecture.md)** - System architecture documentation
  - Overview of SPLAT!'s system architecture
  - Component interactions and data flow
  - System design diagrams (Mermaid format)

- **[data_file_formats.md](data_file_formats.md)** - Complete data file format reference
  - Detailed documentation of all SPLAT! data file formats
  - File structure, examples, and usage guidelines
  - Includes: QTH, LRP, SDF, AZ, EL, LCF, SCF, DCF, ANO files
  - Data flow diagrams (Mermaid format)

### Manual Pages

The `manual/` directory contains the official SPLAT! manual pages in multiple languages:

- **English** (`manual/english/`) - Complete manual page in English
- **Spanish** (`manual/spanish/`) - Manual page in Spanish (Español)
- **French** (`manual/french/`) - Manual page in French (Français)
- **German** (`manual/german/`) - Manual page in German (Deutsch)

Each language directory contains:
- `splat.man` - The manual page in troff/groff format (for `man` command)
- `CMakeLists.txt` - Build configuration to generate various output formats

## Building Documentation

The documentation can be built using CMake. The build system generates:

- **PostScript** (`.ps`) files from manual pages
- **ASCII text** (`.txt`) files from manual pages
- **HTML** versions (if configured)

To build the documentation:

```bash
mkdir build
cd build
cmake ..
make
```

Specific targets are available for each language:
- `postscript-en` - English manual in PostScript
- `postscript-es` - Spanish manual in PostScript
- `postscript-fr` - French manual in PostScript
- `postscript-de` - German manual in PostScript

## Quick Links

### For New Users

1. Start with the [English manual page](manual/english/splat.md) (Markdown version)
2. Review [data_file_formats.md](data_file_formats.md) to understand input file formats
3. Check [architecture.md](architecture.md) for system overview

### For Developers

1. [architecture.md](architecture.md) - System design and component architecture
2. [data_file_formats.md](data_file_formats.md) - File format specifications

### For Users in Other Languages

- [Spanish Manual](manual/spanish/splat.man) - Manual en Español
- [French Manual](manual/french/splat.man) - Manuel en Français
- [German Manual](manual/german/splat.man) - Handbuch auf Deutsch

## File Formats

SPLAT! uses several file formats for input and output:

### Required Input Files
- **QTH files** (`.qth`) - Site location and antenna height
- **LRP files** (`.lrp`) - Irregular Terrain Model parameters
- **SDF files** (`.sdf` or `.sdf.bz2`) - Topographic elevation data

### Optional Input Files
- **AZ files** (`.az`) - Azimuth antenna radiation patterns
- **EL files** (`.el`) - Elevation antenna radiation patterns
- **LCF files** (`.lcf`) - Path loss color definitions
- **SCF files** (`.scf`) - Signal strength color definitions
- **DCF files** (`.dcf`) - Received power level color definitions

### Output Files
- **ANO files** (`.ano`) - Alphanumeric contour data export
- **Image files** (`.png`, `.jpg`, `.ppm`, `.tif`) - Topographic and coverage maps
- **KML/KMZ files** - Google Earth compatible output
- **GEO files** (`.geo`) - Xastir georeference files

For detailed information on all file formats, see [data_file_formats.md](data_file_formats.md).

## Additional Resources

- **Official Website**: http://www.qsl.net/kd2bd/splat.html
- **Version**: 2.0.0
- **License**: GNU General Public License, Version 2

## Contributing

Documentation improvements are welcome! If you find errors or have suggestions:

1. Check the existing documentation structure
2. Follow the existing format and style
3. Update all language versions if applicable
4. Include diagrams in Mermaid format when appropriate

## Notes

- All manual pages are synchronized across languages
- The English version is considered the authoritative source
- Markdown documentation files use GitHub-compatible Mermaid diagrams
- Manual pages use troff/groff macros for Unix/Linux `man` command compatibility

