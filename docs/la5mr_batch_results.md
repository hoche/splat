# LA5MR limited splat-batch results

This file records a limited `splat-batch` run for the LA5MR network only.
Numbers are from that run. Full Norway was not run.

Related acceptance notes: [test_results.md](test_results.md).

## Scope

- **Network:** OSM [relation/18780801](https://www.openstreetmap.org/relation/18780801)
  (`name=LA5MR`, 11 members)
- **Positions:** from the project joz repeater list (not OSM coordinates when they differ)
- **Excluded:** Bagn `node/2641537344`; replaced by Høykorset (`node/1705107664`)
- **Not run:** full national / Norway-wide batch

Changeset context for related Innlandet uploads:
https://www.openstreetmap.org/changeset/189847435

## Jobs (11)

Simulcast TX sites share callsign `LA5MR`, so batch keys are uniquified
(`LA5MR_<id|site>`).

| # | Job key | Callsign | Lat | Lon |
|---|---------|----------|-----|-----|
| 1 | `LA5TRR` | LA5TRR | 61.3193750 | 12.1901470 |
| 2 | `LA5MR_1473491257` | LA5MR | 60.8457632 | 10.8961777 |
| 3 | `LA2TRR` | LA2TRR | 61.0075560 | 10.1918000 |
| 4 | `LA5MR_Hoykorset` | LA5MR | 60.4781659 | 10.6018219 |
| 5 | `LA5MR_Bislingen` | LA5MR | 60.2298669 | 10.6066179 |
| 6 | `LA5MR_5576503609` | LA5MR | 61.0184625 | 8.9739277 |
| 7 | `LA5MR_12635462528` | LA5MR | 61.2532188 | 8.2030446 |
| 8 | `LA9AR` | LA9AR | 62.1741650 | 10.6952740 |
| 9 | `LA2KRR` | LA2KRR | 60.3242222 | 12.1383250 |
| 10 | `LA6JRR` | LA6JRR | 61.8978325 | 9.2835382 |
| 11 | `LA6NR` | LA6NR | 60.5366765 | 9.0617188 |

## Results

| Metric | Value |
|--------|-------|
| Output directory (ephemeral) | `/tmp/splat-batch-la5mr` |
| Checked-in copies | [`test-results/`](../test-results/) |
| Jobs | **11 run**, **0 failed**, **0 skipped** |
| Elapsed | ~1645 s (~27 min) |
| Terrain | Mapterhorn (`planet.pmtiles`) |
| ITM | error **3** on all 11 (kept; does not fail the job) |
| County outputs | `Innlandet.{geojson,fgb}`, `Buskerud.*`, `Unknown.*` (LA6JRR) |

County GeoJSON/FlatGeobuf (Innlandet compressed with bzip2), Google Earth
KML/KMZ, a coverage-map sample, and gnuplot path-profile samples are in
[`test-results/`](../test-results/) (see that folder's README). The original
batch tree under `/tmp` is ephemeral. Feature GeoJSON (hear/talk) was written
under `features/` in that directory; one small sample is checked in.

## Not claimed

- A completed end-to-end national batch of all jobs
- Long-duration production stability beyond this limited run
- Metrics other than those listed above (coverage areas, path-loss deltas, etc.)

Attribution for Mapterhorn terrain data:
https://mapterhorn.com/attribution
