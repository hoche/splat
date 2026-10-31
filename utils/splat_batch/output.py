"""Per-county GeoJSON and FlatGeobuf output."""

import json
import logging
import os
import re
import subprocess
from collections import defaultdict

log = logging.getLogger(__name__)

_FORMAT_SUFFIX = {"geojson": ".geojson", "flatgeobuf": ".fgb"}


class OutputError(Exception):
    """Raised when an output file cannot be written."""


def county_filename(county):
    """Filesystem-safe base name for a county."""
    name = re.sub(r"[^\w.-]+", "_", county.strip(), flags=re.UNICODE)
    return name.strip("._") or "unknown"


def _load_features(path):
    with open(path, "r", encoding="utf-8") as handle:
        return json.load(handle).get("features", [])


def _write_flatgeobuf(geojson_path, fgb_path, layer, config):
    if os.path.exists(fgb_path):
        os.remove(fgb_path)
    cmd = [config["tools"]["ogr2ogr"], "-f", "FlatGeobuf", "-nln", layer,
           "-a_srs", "EPSG:4326", "-nlt", "MULTIPOLYGON", fgb_path,
           geojson_path]
    try:
        proc = subprocess.run(cmd, stdout=subprocess.PIPE,
                              stderr=subprocess.STDOUT,
                              universal_newlines=True)
    except OSError as exc:
        raise OutputError("cannot run %s: %s" % (cmd[0], exc))
    if proc.returncode != 0:
        raise OutputError("ogr2ogr failed for %s: %s"
                          % (fgb_path, proc.stdout.strip()))


def write_county_outputs(jobs, out_dir, config):
    """Write one GeoJSON/FlatGeobuf per county from the jobs' feature files.

    Returns a list of (path, size_bytes) for every file written.  Jobs
    without a feature file are ignored.
    """
    by_county = defaultdict(list)
    for job in jobs:
        if not os.path.exists(job.features_file):
            continue
        for feature in _load_features(job.features_file):
            by_county[job.county].append(feature)

    written = []
    formats = config["output"]["formats"]
    for county, features in sorted(by_county.items()):
        features.sort(key=lambda f: (f["properties"]["callsign"],
                                     f["properties"]["frequency"],
                                     f["properties"]["level"]))
        base = county_filename(county)
        geojson_path = os.path.join(out_dir, base + ".geojson")
        # The GeoJSON file is always produced; it is the source for FlatGeobuf.
        tmp = geojson_path + ".tmp"
        with open(tmp, "w", encoding="utf-8") as handle:
            json.dump({"type": "FeatureCollection", "name": county,
                       "features": features}, handle, separators=(",", ":"))
        os.replace(tmp, geojson_path)

        if "flatgeobuf" in formats:
            fgb_path = os.path.join(out_dir, base + _FORMAT_SUFFIX["flatgeobuf"])
            _write_flatgeobuf(geojson_path, fgb_path, base, config)
            written.append((fgb_path, os.path.getsize(fgb_path)))

        if "geojson" in formats:
            written.append((geojson_path, os.path.getsize(geojson_path)))
        else:
            os.remove(geojson_path)
        log.info("county %s: %d features", county, len(features))
    return written
