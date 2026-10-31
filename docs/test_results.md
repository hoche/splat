# Acceptance and verification results

This file records verification performed during Mapterhorn terrain support
and `splat-batch` development. Numbers are from those runs only. Items that
were not completed are marked as such.

How to run the automated unit suite and sanitizers is described in
[TESTING.md](../TESTING.md).

## Unit tests

- **306 passed**, **3 skipped**
- Skips: PPD stubs and no GDAL GeoTIFF support in the test environment

## Mapterhorn / WebP terrain

- WebP tiles confirmed **lossless** Terrarium-encoded elevation (as used by
  Mapterhorn)
- SDF tile **60/10** (page `60_61_349_350`): header and lake/summit checks for
  Tyrifjorden, Randsfjorden, Lushaugen, and Skreikampen passed
- Path-loss comparison at **145 MHz**: SRTM/Skadi vs Mapterhorn within about
  **1.3 dB**
- Builds **without** `-mapterhorn`: output **byte-identical** to unpatched HEAD

## Despike (`mapterhorn2sdf`)

- Randsfjorden spike reduced from **494 m** to **133 m**
- Real summits on the 60/10 page preserved (not clipped relative to
  `--no-despike`)

## splat-batch

- Dry-run on `repeaters.joz`: **236 jobs** / **245 skips**
- SD vs HD hear area on four hilly sites: about **+0.1% to +3.9%** (SD larger)
- ITM coverage error reporting: paths under 2 km ignored; error **3** counted
  in the run report; errors **>= 4** fail the job
- **Full Norway / national 236-job production run was not completed**
- Limited LA5MR network batch (OSM relation/18780801, 11 jobs, joz positions):
  **11 run**, **0 failed**, **0 skipped**; elapsed **~1645 s**; ITM error **3**
  on all 11 (kept; does not fail the job); county outputs Innlandet, Buskerud,
  Unknown (LA6JRR). Full job list and notes:
  [la5mr_batch_results.md](la5mr_batch_results.md). Graphical and county
  output files: [`test-results/`](../test-results/). Overview also linked from
  the top-level [README.md](../README.md).

## Bitihorn inland coverage

Single-site ITWOM path-loss run at Bitihorn (Innlandet; 61.2943895 N,
8.7991108 E; 6 m AGL; 145.700 MHz; RX 1.5 m; 100 km; Mapterhorn), using inland
`.lrp` climate **5** and average ground rather than the maritime climate-7
default. Simplified hear/talk areas were **13737.67 km2** / **9759.82 km2**.
Marble-ready outputs: [`test-results/Bitihorn.geojson`](../test-results/Bitihorn.geojson),
`.kml`, and `.kmz` (site Point included). Full notes lived under
`/tmp/splat-bitihorn/ANALYSIS.md`. Replaces the earlier Galdhøpiggen example.

## Not verified / not claimed

- A completed end-to-end national / Norway-wide batch of all 236 jobs
- Long-duration production stability metrics beyond the checks above

Attribution for Mapterhorn terrain data:
https://mapterhorn.com/attribution
