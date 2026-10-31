# LA5MR limited-batch test results

Checked-in outputs from the limited `splat-batch` run for OSM
[relation/18780801](https://www.openstreetmap.org/relation/18780801)
(LA5MR network, 11 jobs, joz positions). Full Norway was not run.

Narrative and job table: [docs/la5mr_batch_results.md](../docs/la5mr_batch_results.md).
Acceptance notes: [docs/test_results.md](../docs/test_results.md).

Terrain: Mapterhorn. Attribution (required):
https://mapterhorn.com/attribution
(see also `ATTRIBUTION.txt`).

## Viewing in KDE Marble

Prefer `LA5MR.kmz` (or decompress `LA5MR.kml.bz2` / `LA5MR.geojson.bz2`).
Polygons use simple-style GeoJSON properties (`stroke`, `fill`,
`fill-opacity`, `name`/`title`) and matching KML `PolyStyle`/`LineStyle`
colors (AABBGGRR). Hear polygons use fill-opacity 0.40; talk use 0.22.

Each file also includes **transmitter site Point features / Placemarks**
(joz lon/lat) with `properties.name` / KML `<name>` so Marble can label
sites. Coverage polygons use the same display names plus `(hear)` /
`(talk)`. KML groups polygons under a `Coverage` folder and sites under
`Transmitter sites`.

```bash
bzip2 -dk LA5MR.geojson.bz2   # ~195 MiB uncompressed
bzip2 -dk LA5MR.kml.bz2       # ~259 MiB uncompressed
```

## Color legend (display name)

Same palette across GeoJSON, KML, and KMZ. Job keys remain in the
`callsign` property for coloring; Marble labels use the display name.

| Display name | Job key | Color | Hex |
|--------------|---------|-------|-----|
| LA5TRR (Trysilfjellet) | LA5TRR | red | `#e41a1c` |
| LA5MR (Høykorset) | LA5MR_HOYKORSET | blue | `#377eb8` |
| LA5MR (Bislingen) | LA5MR_BISLINGEN | green | `#4daf4a` |
| LA5MR (1473491257) | LA5MR_1473491257 | purple | `#984ea3` |
| LA5MR (5576503609) | LA5MR_5576503609 | orange | `#ff7f00` |
| LA5MR (12635462528) | LA5MR_12635462528 | brown | `#a65628` |
| LA2TRR (Kråkhugukampen) | LA2TRR | pink | `#f781bf` |
| LA9AR (Tronfjell) | LA9AR | teal | `#66c2a5` |
| LA2KRR (Rafjell) | LA2KRR | coral | `#fc8d62` |
| LA6JRR | LA6JRR | periwinkle | `#8da0cb` |
| LA6NR (Syningen) | LA6NR | gold | `#e6ab02` |

Coverage features also carry `callsign`, `site_name`, `job_id`,
`level` (hear/talk), `coverage_type`, `frequency`, `band`, and `county`.
Site Points use `feature_role=transmitter` plus matching `marker-color`.

## Present

### Network merge (all 11 jobs)

| File | Notes |
|------|--------|
| `LA5MR.geojson.bz2` | 22 hear/talk polygons + 11 transmitter Points; ~195 MiB uncompressed (bz2 for GitHub) |
| `LA5MR.kml.bz2` | Same with KML styles + site Placemarks; ~259 MiB uncompressed |
| `LA5MR.kmz` | Zipped KML (~32 MiB); convenient for Marble / Google Earth |

### County polygons (from `/tmp/splat-batch-la5mr`, colorized)

| File | Notes |
|------|--------|
| `Innlandet.geojson.bz2` | Colorized GeoJSON; original ~161 MiB |
| `Innlandet.fgb.bz2` | FlatGeobuf (geometry only; colors live in GeoJSON/KML) |
| `Innlandet.kml.bz2` | Colorized KML for Marble / Google Earth |
| `Buskerud.{geojson,fgb,kml,kmz}` | LA6NR; colorized GeoJSON/KML/KMZ |
| `Unknown.{geojson,fgb,kml,kmz}` | LA6JRR; colorized GeoJSON/KML/KMZ |
| `features/LA5MR_12635462528_145625.geojson` | Per-job hear/talk sample (colorized) |
| `report.json`, `splat-batch.log`, `ATTRIBUTION.txt` | Batch metadata |

Decompress Innlandet GeoJSON with:

```bash
bzip2 -dk Innlandet.geojson.bz2
```

### Samples regenerated after the batch (SDF cache `/tmp/sdf-batch-cache`)

`splat-batch` discards per-job work dirs (`site.kml` / coverage images) by
default, so Google Earth overlays and coverage PNGs from the original jobs
were not retained. These samples were regenerated with `/tmp/splat-build/splat`
and the existing SDF cache (no full Norway re-run):

| Path | What |
|------|------|
| `samples/LA5MR_12635462528_site.png` | Path-loss coverage map (100 km, RX 1.5 m) |
| `samples/LA5MR_12635462528_site.kml` | SPLAT! LatLonBox KML for that map |
| `samples/hoykorset_bislingen_*.png` | gnuplot terrain / elevation / height profiles |
| `samples/hoykorset_bislingen_*.gp` | Preserved gnuplot scripts (`-gpsav`) |
| `samples/hoykorset_bislingen_LA5MR_Hoykorset-to-LA5MR_Bislingen.kml` | Point-to-point Google Earth link |

Path-profile TX/RX: LA5MR Hoykorset and Bislingen (positions from the LA5MR
job table). TX height used the batch default (6 m AGL) for the sample.

### Bitihorn inland single-site (climate 5)

Separate from the LA5MR network batch: one alpine TX at Bitihorn
(61.2943895 N, 8.7991108 E), 145.700 MHz, RX 1.5 m, radius 100 km,
Mapterhorn terrain. Inland Longley-Rice / ITWOM `.lrp`: radio climate **5**
(Continental Temperate), average ground (dielectric 15, conductivity 0.005).
Not the `splat-batch` default climate 7 (maritime over sea). Replaces the
earlier Galdhøpiggen single-site example.

Simplified polygon areas from the analysis write-up: hear **13737.67 km2**,
talk **9759.82 km2**. Files include a transmitter Point / Placemark named
Bitihorn and Marble-friendly styles (magenta `#e7298a`).

| File | Notes |
|------|--------|
| `Bitihorn.geojson` | Hear/talk MultiPolygons + site Point (~24 MiB; under GitHub limit) |
| `Bitihorn.kml` | Same with Coverage / Transmitter sites folders (~29 MiB) |
| `Bitihorn.kmz` | Zipped KML (~3.7 MiB); convenient for Marble / Google Earth |

Source tree (ephemeral): `/tmp/splat-bitihorn/` (`ANALYSIS.md`).

## Not generated by batch defaults

- Per-job coverage PNGs / PPMs for all 11 sites (work dirs cleaned; only one
  sample regenerated above)
- Uncompressed Innlandet / full LA5MR GeoJSON/KML (stored as `.bz2`; use
  `LA5MR.kmz` for Marble without decompressing)
- Signal-strength graphs for every repeater (gnuplot path profiles need
  point-to-point `-t`/`-r`; batch coverage mode does not invoke them)
- Remaining per-job feature GeoJSON under `features/` (large; one sample kept;
  full merge is `LA5MR.geojson.bz2`)

## Original ephemeral location

The full batch tree (all feature GeoJSON, uncompressed Innlandet, empty
`work/`) was produced under `/tmp/splat-batch-la5mr`.
