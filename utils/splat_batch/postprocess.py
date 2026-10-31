"""Post-processing of SPLAT! path-loss images into hear/talk polygons.

Pipeline for one job:

1. Read the bounding box from the KML written by SPLAT! (-kml).
2. Read the P6 PPM path-loss image and recover two binary masks by exact
   colour match against the colours written to the job's .lcf file.  The
   terrain background is grey, so non-grey LCF colours are unambiguous.
   The hear mask covers pixels coloured talk or hear (talk is the inner
   region and lies inside hear).
3. Crop each mask to its data extent and georeference it with
   ``gdal_translate -a_srs EPSG:4326 -a_ullr``.
4. Polygonize with ``gdal_polygonize`` (GDAL CLI).
5. Simplify inwards only: the polygon is simplified in a local metric
   projection and intersected with the original, so the result never
   extends beyond the unsimplified coverage.  Parts below the minimum area
   are dropped.

Requires numpy, shapely and the GDAL command-line tools gdal_translate and
gdal_polygonize(.py).
"""

import json
import logging
import math
import os
import re
import shutil
import subprocess
import xml.etree.ElementTree as ET

log = logging.getLogger(__name__)

_CHUNK_ROWS = 512

# Local equirectangular projection constants (metres per degree).
_M_PER_DEG_LAT = 110574.0
_M_PER_DEG_LON_EQ = 111320.0


class PostprocessError(Exception):
    """Raised when a job's output cannot be converted."""


# ---------------------------------------------------------------------------
# Tool discovery
# ---------------------------------------------------------------------------

def find_polygonize(config):
    """Locate gdal_polygonize.py (or gdal_polygonize); return path or None."""
    configured = config["tools"].get("gdal_polygonize")
    candidates = [configured] if configured else [
        "gdal_polygonize.py", "gdal_polygonize"]
    for candidate in candidates:
        found = shutil.which(candidate)
        if found:
            return found
    return None


def _run_tool(cmd):
    try:
        proc = subprocess.run(cmd, stdout=subprocess.PIPE,
                              stderr=subprocess.STDOUT,
                              universal_newlines=True)
    except OSError as exc:
        raise PostprocessError("cannot run %s: %s" % (cmd[0], exc))
    if proc.returncode != 0:
        raise PostprocessError("%s failed (status %d): %s"
                               % (os.path.basename(cmd[0]), proc.returncode,
                                  proc.stdout.strip()))


# ---------------------------------------------------------------------------
# KML and PPM reading
# ---------------------------------------------------------------------------

def parse_kml_bounds(path):
    """Return (north, south, east, west) from the KML LatLonBox."""
    try:
        root = ET.parse(path).getroot()
    except (OSError, ET.ParseError) as exc:
        raise PostprocessError("cannot read KML %s: %s" % (path, exc))
    values = {}
    for elem in root.iter():
        name = elem.tag.rsplit("}", 1)[-1]
        if name in ("north", "south", "east", "west") and elem.text:
            values[name] = float(elem.text)
    missing = {"north", "south", "east", "west"} - set(values)
    if missing:
        raise PostprocessError("KML %s lacks LatLonBox values: %s"
                               % (path, ", ".join(sorted(missing))))
    return values["north"], values["south"], values["east"], values["west"]


def read_ppm_header(path):
    """Return (width, height, data_offset) of a binary P6 PPM file."""
    with open(path, "rb") as handle:
        head = handle.read(256)
    tokens = []
    pos = 0
    while len(tokens) < 4:
        match = re.compile(rb"\s*(#[^\n]*\n\s*)*(\S+)").match(head, pos)
        if not match:
            raise PostprocessError("%s: malformed PPM header" % path)
        tokens.append(match.group(2))
        pos = match.end()
    if tokens[0] != b"P6":
        raise PostprocessError("%s is not a binary P6 PPM" % path)
    width, height, maxval = (int(t) for t in tokens[1:4])
    if maxval != 255:
        raise PostprocessError("%s: unsupported maxval %d" % (path, maxval))
    # Exactly one whitespace byte separates the header from the data.
    return width, height, pos + 1


def build_masks(ppm_path, talk_rgb, hear_rgb):
    """Return (hear_mask, talk_mask) uint8 arrays from the PPM image."""
    import numpy as np

    width, height, offset = read_ppm_header(ppm_path)
    expected = offset + width * height * 3
    actual = os.path.getsize(ppm_path)
    if actual < expected:
        raise PostprocessError("%s is truncated (%d < %d bytes)"
                               % (ppm_path, actual, expected))

    image = np.memmap(ppm_path, dtype=np.uint8, mode="r", offset=offset,
                      shape=(height, width, 3))
    hear = np.zeros((height, width), dtype=np.uint8)
    talk = np.zeros((height, width), dtype=np.uint8)
    for start in range(0, height, _CHUNK_ROWS):
        block = image[start:start + _CHUNK_ROWS]
        is_talk = ((block[..., 0] == talk_rgb[0]) &
                   (block[..., 1] == talk_rgb[1]) &
                   (block[..., 2] == talk_rgb[2]))
        is_hear = ((block[..., 0] == hear_rgb[0]) &
                   (block[..., 1] == hear_rgb[1]) &
                   (block[..., 2] == hear_rgb[2]))
        talk[start:start + _CHUNK_ROWS] = is_talk
        hear[start:start + _CHUNK_ROWS] = is_talk | is_hear
    del image
    return hear, talk


# ---------------------------------------------------------------------------
# Raster to polygons
# ---------------------------------------------------------------------------

def _write_pgm(path, array):
    height, width = array.shape
    with open(path, "wb") as handle:
        handle.write(b"P5\n%d %d\n255\n" % (width, height))
        handle.write(array.tobytes())


def mask_to_polygon_geometries(mask, bounds, workdir, name, config):
    """Polygonize a binary mask; return GeoJSON geometry dicts (lon/lat)."""
    import numpy as np

    rows = np.flatnonzero(mask.any(axis=1))
    if rows.size == 0:
        return []
    cols = np.flatnonzero(mask.any(axis=0))
    r0, r1 = int(rows[0]), int(rows[-1])
    c0, c1 = int(cols[0]), int(cols[-1])

    north, south, east, west = bounds
    height, width = mask.shape
    px = (east - west) / width
    py = (north - south) / height
    ullr = (west + c0 * px, north - r0 * py,
            west + (c1 + 1) * px, north - (r1 + 1) * py)

    pgm = os.path.join(workdir, name + ".pgm")
    tif = os.path.join(workdir, name + ".tif")
    out = os.path.join(workdir, name + "_poly.geojson")
    _write_pgm(pgm, np.ascontiguousarray(mask[r0:r1 + 1, c0:c1 + 1]))

    _run_tool([config["tools"]["gdal_translate"], "-q", "-of", "GTiff",
               "-a_srs", "EPSG:4326",
               "-a_ullr"] + ["%.10f" % v for v in ullr] +
              ["-co", "COMPRESS=DEFLATE", pgm, tif])

    polygonize = find_polygonize(config)
    if polygonize is None:
        raise PostprocessError("gdal_polygonize not found")
    if os.path.exists(out):
        os.remove(out)
    _run_tool([polygonize, "-q", "-mask", tif, tif, "-f", "GeoJSON", out])

    if not os.path.exists(out):
        return []
    with open(out, "r", encoding="utf-8") as handle:
        doc = json.load(handle)
    return [f["geometry"] for f in doc.get("features", [])
            if f.get("geometry") and
            (f.get("properties") or {}).get("DN", 1) == 1]


# ---------------------------------------------------------------------------
# Geometry clean-up
# ---------------------------------------------------------------------------

def _polygon_parts(geometry):
    """Yield the Polygon parts of any shapely geometry."""
    if geometry.is_empty:
        return
    kind = geometry.geom_type
    if kind == "Polygon":
        yield geometry
    elif kind in ("MultiPolygon", "GeometryCollection"):
        for part in geometry.geoms:
            for poly in _polygon_parts(part):
                yield poly


def _repair(geometry):
    if geometry.is_valid:
        return geometry
    try:
        from shapely.validation import make_valid
        return make_valid(geometry)
    except ImportError:
        return geometry.buffer(0)


def simplify_inward(polygon, tolerance_m):
    """Simplify polygon but never extend beyond it (result is a subset).

    Uses non-topology-preserving Douglas-Peucker for stronger vertex
    reduction, then intersects with the original so the result cannot grow.
    """
    polygon = _repair(polygon)
    if tolerance_m <= 0:
        return polygon
    simplified = _repair(polygon.simplify(tolerance_m, preserve_topology=False))
    return _repair(simplified.intersection(polygon))


def clean_geometries(geojson_geometries, lat0, lon0, config):
    """Simplify inwards, drop small islands; return a lon/lat geometry.

    Returns a shapely MultiPolygon, or None when nothing remains.
    """
    import numpy as np
    from shapely.geometry import MultiPolygon, shape
    from shapely.ops import transform

    post = config["postprocess"]
    tolerance = float(post["simplify_tolerance_m"])
    min_area = float(post["min_area_ha"]) * 10000.0
    decimals = int(post["coord_decimals"])

    kx = _M_PER_DEG_LON_EQ * math.cos(math.radians(lat0))
    ky = _M_PER_DEG_LAT

    def to_metres(x, y, z=None):
        return ((np.asarray(x) - lon0) * kx, (np.asarray(y) - lat0) * ky)

    def to_degrees(x, y, z=None):
        return (np.round(np.asarray(x) / kx + lon0, decimals),
                np.round(np.asarray(y) / ky + lat0, decimals))

    kept = []
    for raw in geojson_geometries:
        poly_m = transform(to_metres, shape(raw))
        for part in _polygon_parts(_repair(poly_m)):
            if part.area < min_area:
                continue
            for piece in _polygon_parts(simplify_inward(part, tolerance)):
                if piece.area >= min_area:
                    kept.append(piece)
    if not kept:
        return None

    result = []
    for piece in kept:
        for poly in _polygon_parts(_repair(transform(to_degrees, piece))):
            result.append(poly)
    if not result:
        return None
    return MultiPolygon(result)


# ---------------------------------------------------------------------------
# Job entry point
# ---------------------------------------------------------------------------

def postprocess_job(job, config):
    """Convert a finished SPLAT! run into GeoJSON features (list of dicts)."""
    from shapely.geometry import mapping

    ppm = os.path.join(job.workdir, "site.ppm")
    kml = os.path.join(job.workdir, "site.kml")
    for path in (ppm, kml):
        if not os.path.exists(path):
            raise PostprocessError("expected SPLAT! output missing: %s"
                                   % os.path.basename(path))

    bounds = parse_kml_bounds(kml)
    colors = config["colors"]
    hear_mask, talk_mask = build_masks(ppm, colors["talk"], colors["hear"])

    features = []
    for level, mask in (("hear", hear_mask), ("talk", talk_mask)):
        geoms = mask_to_polygon_geometries(mask, bounds, job.workdir, level,
                                           config)
        geometry = clean_geometries(geoms, job.lat, job.lon, config)
        if geometry is None:
            log.info("%s: no %s area above the minimum size", job.job_id,
                     level)
            continue
        features.append({
            "type": "Feature",
            "properties": {
                "callsign": job.callsign,
                "frequency": job.frequency_mhz,
                "band": job.band,
                "county": job.county,
                "level": level,
                "params_version": str(config["params_version"]),
                "terrain": "Mapterhorn",
                "attribution":
                    "https://mapterhorn.com/attribution",
            },
            "geometry": mapping(geometry),
        })
    return features
