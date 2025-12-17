# SPLAT! 2.0 Command-Line Quick Reference

## What Changed?

Starting with SPLAT! 2.0, multi-character options require **double dashes (`--`)** instead of single dash (`-`).

## Quick Conversion Table

| Old (1.x) | New (2.0) | Description |
|-----------|-----------|-------------|
| `-t` | `-t, --transmitter` | Transmitter site(s) |
| `-r` | `-r, --receiver` | Receiver site |
| `-c` | `-c, --coverage` | Coverage analysis |
| `-L` | `-L, --pathloss` | Path loss map |
| `-s` | `-s, --cities` | City files |
| `-b` | `-b, --boundaries` | Boundary files |
| `-p` | `-p, --terrain-profile` | Terrain profile |
| `-e` | `-e, --elevation-plot` | Elevation plot |
| `-h` | `--height-plot` | Height plot (no `-h`) |
| `-H` | `-H, --normalized-height` | Normalized height |
| `-l` | `-l, --longley-plot` | Longley plot |
| `-o` | `-o, --output-map` | Output map |
| `-d` | `-d, --sdf-path` | SDF directory |
| `-m` | `-m, --earth-multiplier` | Earth radius multiplier |
| `-n` | `-n, --no-los-path` | No LOS path |
| `-N` | `-N, --no-reports` | No reports |
| `-f` | `-f, --frequency` | Frequency (MHz) |
| `-R` | `-R, --range` | Maximum range |
| `-v` | `-v, --verbose` | Verbosity level |
| `-hd` | `--hd, --high-definition` | HD mode ⚠️ |
| `-st` | `--st, --single-thread` | Single thread ⚠️ |
| `-sc` | `--sc, --smooth-contours` | Smooth contours ⚠️ |
| `-db` | `--db, --contour-threshold` | Contour threshold ⚠️ |
| `-nf` | `--nf, --no-fresnel` | No Fresnel zones ⚠️ |
| `-fz` | `--fz, --fresnel-clearance` | Fresnel clearance ⚠️ |
| `-gc` | `--gc, --ground-clutter` | Ground clutter ⚠️ |
| `-ppm` | `--ppm` | PPM format ⚠️ |
| `-jpg` | `--jpg` | JPEG format ⚠️ |
| `-tif` | `--tif` | GeoTIFF format ⚠️ |
| `-ngs` | `--ngs, --no-greyscale` | No greyscale ⚠️ |
| `-erp` | `--erp, --effective-power` | Effective power ⚠️ |
| `-ano` | `--ano, --alphanum-output` | Alphanum output ⚠️ |
| `-ani` | `--ani, --alphanum-input` | Alphanum input ⚠️ |
| `-udt` | `--udt, --user-terrain` | User terrain ⚠️ |
| `-kml` | `--kml` | KML output ⚠️ |
| `-kmz` | `--kmz` | KMZ output ⚠️ |
| `-geo` | `--geo` | GEO file ⚠️ |
| `-dbm` | `--dbm` | dBm output ⚠️ |
| `-log` | `--log, --command-log` | Log file ⚠️ |
| `-json` | `--json` | JSON output ⚠️ |
| `-gpsav` | `--gpsav` | Save gnuplot files ⚠️ |
| `-itwom` | `--itwom` | ITWOM model ⚠️ |
| `-imperial` | `--imperial` | Imperial units ⚠️ |
| `-msl` | `--msl` | MSL altitudes ⚠️ |
| `-maxpages` | `--maxpages` | Max pages ⚠️ |
| `-sdelim` | `--sdelim, --sdf-delimiter` | SDF delimiter ⚠️ |

⚠️ = **Changed** - Now requires `--` prefix

## Example Conversions

### Before (SPLAT! 1.x):
```bash
splat -t wnjt-dt.qth -r receiver.qth -c 10 -hd -sc -o coverage
```

### After (SPLAT! 2.0):
```bash
splat -t wnjt-dt.qth -r receiver.qth -c 10 --hd --sc -o coverage
```

---

### Before (SPLAT! 1.x):
```bash
splat -t tx.qth -L 25 -f 915 -erp 1000 -itwom -maxpages 36
```

### After (SPLAT! 2.0):
```bash
splat -t tx.qth -L 25 -f 915 --erp 1000 --itwom --maxpages 36
```

---

### Before (SPLAT! 1.x):
```bash
splat -t tx.qth -r rx.qth -h height.png -nf -gc 30
```

### After (SPLAT! 2.0):
```bash
splat -t tx.qth -r rx.qth --height-plot height.png --nf --gc 30
```

## Getting Help

Run `splat --help` for a complete list of options with descriptions.

Run `splat --version` to see the version number.

## Why the Change?

CLI11 is a modern, well-maintained C++ command-line parser that provides:
- Better error messages
- Automatic help formatting
- Type validation
- Mutual exclusion handling
- Professional user experience

The double-dash convention for multi-character options is standard in modern CLI tools (git, docker, kubectl, etc.).

## Need More Help?

- See `CLI11_MIGRATION.md` for detailed migration information
- See `docs/manual/english/splat.md` for full documentation
- Run `man splat` for the manual page



