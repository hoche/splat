"""Terrain prefetch through mapterhorn2sdf, and terrain identity for caching."""

import logging
import math
import os
import subprocess

from .jobs import job_bbox

log = logging.getLogger(__name__)

_EDGE_EPSILON = 1.0e-4


class PrefetchError(Exception):
    """Raised when mapterhorn2sdf fails."""


def terrain_token(config):
    """A string identifying the terrain source and zoom, for cache keys."""
    mt = config["mapterhorn"]
    if not mt["enabled"]:
        return "sdf"
    source = str(mt["source"])
    token = "mapterhorn|%s|zoom=%s" % (source, mt.get("zoom"))
    if os.path.isfile(source):
        stat = os.stat(source)
        token += "|size=%d|mtime_ns=%d" % (stat.st_size, stat.st_mtime_ns)
    return token


def job_cells(job, margin_deg):
    """Set of (lat, lon) integer-degree cells covered by the job bbox."""
    min_lat, min_lon, max_lat, max_lon = job_bbox(job, margin_deg)
    cells = set()
    for lat in range(int(math.floor(min_lat)), int(math.floor(max_lat)) + 1):
        for lon in range(int(math.floor(min_lon)),
                         int(math.floor(max_lon)) + 1):
            cells.add((lat, lon))
    return cells


def cover_rectangles(cells):
    """Cover a set of unit cells by a small set of rectangles.

    Cells are merged into contiguous longitude runs per latitude row, and
    rows with identical runs on consecutive latitudes are stacked.  Returns
    (lat0, lon0, lat1, lon1) tuples in cell indices (inclusive).
    """
    rows = {}
    for lat, lon in cells:
        rows.setdefault(lat, []).append(lon)

    runs = {}
    for lat, lons in rows.items():
        lons.sort()
        start = prev = lons[0]
        for lon in lons[1:]:
            if lon != prev + 1:
                runs.setdefault((start, prev), []).append(lat)
                start = lon
            prev = lon
        runs.setdefault((start, prev), []).append(lat)

    rectangles = []
    for (lon0, lon1), lats in runs.items():
        lats.sort()
        start = prev = lats[0]
        for lat in lats[1:]:
            if lat != prev + 1:
                rectangles.append((start, lon0, prev, lon1))
                start = lat
            prev = lat
        rectangles.append((start, lon0, prev, lon1))
    return sorted(rectangles)


def prefetch_commands(jobs, config, workers):
    """Build one mapterhorn2sdf command per covering rectangle."""
    mt = config["mapterhorn"]
    margin = mt["bbox_margin_deg"]
    cells = set()
    for job in jobs:
        cells |= job_cells(job, margin)

    commands = []
    for lat0, lon0, lat1, lon1 in cover_rectangles(cells):
        bbox = "%d,%d,%.4f,%.4f" % (lat0, lon0, lat1 + 1 - _EDGE_EPSILON,
                                    lon1 + 1 - _EDGE_EPSILON)
        cmd = [mt["bin"]]
        if config["hd"]:
            cmd.append("--hd")
        if mt.get("zoom") is not None:
            cmd += ["--zoom", str(mt["zoom"])]
        cmd += ["--source", str(mt["source"]),
                "--outdir", str(mt["cache"]),
                "--workers", str(workers),
                "--bz2",
                "--bbox", bbox]
        commands.append(cmd)
    return commands


def prefetch(jobs, config, workers):
    """Fetch terrain once for the union of all job bounding boxes."""
    if not jobs:
        return
    os.makedirs(config["mapterhorn"]["cache"], exist_ok=True)
    commands = prefetch_commands(jobs, config, workers)
    log.info("terrain prefetch: %d request(s) for %d job(s)", len(commands),
             len(jobs))
    for cmd in commands:
        log.info("running: %s", " ".join(cmd))
        try:
            proc = subprocess.run(cmd)
        except OSError as exc:
            raise PrefetchError("cannot run %s: %s" % (cmd[0], exc))
        if proc.returncode != 0:
            raise PrefetchError("%s exited with status %d"
                                % (cmd[0], proc.returncode))
