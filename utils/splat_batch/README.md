# splat-batch

Batch coverage driver for SPLAT!.  One independent SPLAT! run is made per
repeater (callsign plus output frequency); results are not combined by
SPLAT!.  Each run yields "hear" and "talk" polygons, written per county as
GeoJSON and FlatGeobuf.

## Dependencies

- Python 3.7 or later
- numpy and shapely (Python modules)
- GDAL command-line tools: `gdal_translate`, `gdal_polygonize.py` (or
  `gdal_polygonize`), and `ogr2ogr` (FlatGeobuf output only)
- A SPLAT! binary built with PPM output, and for `--mapterhorn` the
  `mapterhorn2sdf` utility

All dependencies are checked at start-up with a clear error message.  No
network access is needed by the driver itself.

## Usage

```
utils/splat-batch --input repeaters.joz --out-dir ./out \
    --config config.json \
    --mapterhorn --mt-source /data/norway.pmtiles --mt-cache /data/sdf \
    --splat-bin ./build/splat \
    --workers 4 --prefetch-workers 2 --radius-km 100
```

Terrain resolution is standard definition by default; add `--hd` for
1-arcsecond.  On hilly Norwegian sites, measured hear areas were only about
0–4% larger at SD than HD; SD remains the default.  Use `--hd` when you need
finer ridge detail.  National HD runs need tens of GB of SDF cache and
produce large polygons.  Inward simplification defaults to 100 m
(`--simplify-m`); raise it if GeoJSON is still huge after HD runs.

Inputs: `.joz` (JOSM session), GeoJSON FeatureCollection, or CSV with the
header `callsign,lat,lon,frequency,modulation,flags,county`.  Use
`--dry-run` to list the jobs and skipped entries without running anything.

The packaged defaults are in `config_default.json`; a user config is merged
over them (lists are replaced).  Raise `params_version` to invalidate all
cached results.

## How a job works

1. `site.qth`, `site.lrp` and `site.lcf` are written to the job directory.
   Longitude is written as degrees west.  Transmit antenna height always
   needs a trailing `m` for metres (for example `6m`); bare numbers in a
   `.qth` file are feet even though this fork defaults to metric CLI units
   (`-L`, `-R`, `-c`).  The LCF holds two levels (talk and hear thresholds)
   with non-grey colours.
2. SPLAT! runs with `-st -t site.qth -L <rx> -R <km> -o site -kml -ppm`.
   Because the LRP has ERP 0, the output is a path-loss image.  `-st`
   (single-threaded) is always passed: the batch already parallelises over
   jobs, and multi-threaded PPM output is nondeterministic, which would
   make cached results unstable.
3. The KML gives the geographic bounds.  The PPM is reduced to binary masks
   by exact colour match against the LCF colours (terrain is grey).
4. Masks are georeferenced with `gdal_translate` and polygonized with
   `gdal_polygonize`.
5. Polygons are simplified inwards only (simplified geometry intersected
   with the original, in a local metric projection), and islands below
   `min_area_ha` are dropped.

Path-loss values are integer dB in SPLAT!, so the hear level written to the
LCF is the hear threshold plus one, making "loss <= threshold" inclusive.

SPLAT! limits the analysis range through `-maxpages`; with
`auto_maxpages` the smallest sufficient value is passed per job.  The same
footprint (with `bbox_margin_deg`) determines the terrain prefetch region,
which is covered by a small set of rectangles rather than one national
bounding box.

## ITM error numbers

SPLAT! prints a Longley-Rice or ITWOM model error number in its log (paths
under 2 km are ignored).  After a successful exit, `splat.log` is searched
for lines such as `Longley-Rice model error number: N` or `ITWOM model
error number: N`; the highest number found is stored as the job's
`itm_error` (0 if none).  The batch treats `itm_error >= 4` as job failure
and counts error 3 in the run report (details below).

- `itm_error >= 4`: the job fails with reason `itm_error_<N>`; it is not
  post-processed and not cached as a success.
- `itm_error == 3`: the job succeeds and is cached, but is flagged.

The run report (text and `report.json`) gives `itm_error_3` (count),
`itm_error_ge_4` (count of jobs failed for this reason) and
`itm_error_3_jobs` (job ids).  The error number is stored in the cache
index, so cached jobs with error 3 are still reported on reruns.

## Cache

`cache_index.json` in the output directory maps each job to a SHA-256 over
the rendered input files, the full SPLAT! command line, the SPLAT! binary
identity, the terrain source and zoom, the post-processing settings and
`params_version`.  Unchanged jobs are skipped; their stored per-job
features (`features/<job>.geojson`) are reused when the county files are
rebuilt.  `--force` ignores the cache.

## Mapterhorn flags

The SPLAT! arguments used for terrain are templates in the `mapterhorn`
config section (`splat_args`, `zoom_args`), so they can be adjusted to the
flags of the SPLAT! build in use.

When you build a regional `norway.pmtiles` with `pmtiles extract`, `--bbox`
is **lon,lat** (`MIN_LON,MIN_LAT,MAX_LON,MAX_LAT`). When you prefetch SDF
pages with `mapterhorn2sdf --bbox`, the box is **lat,lon** (degrees EAST for
lon). See `utils/README.md` for a Norway mainland example and disk planning.
