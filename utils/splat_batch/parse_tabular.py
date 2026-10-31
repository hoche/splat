"""Parsers for GeoJSON FeatureCollection and CSV repeater lists."""

import csv
import json
import logging

from .fields import build_records, parse_float, valid_position
from .models import SkipRecord

log = logging.getLogger(__name__)


def _geometry_position(geometry):
    """Return (lat, lon) for a GeoJSON geometry, or (None, None)."""
    if not geometry:
        return None, None
    if geometry.get("type") == "Point":
        coords = geometry.get("coordinates") or []
        if len(coords) >= 2:
            return float(coords[1]), float(coords[0])
        return None, None
    try:
        from shapely.geometry import shape
        centroid = shape(geometry).centroid
        return centroid.y, centroid.x
    except Exception:
        return None, None


def parse_geojson(path, config):
    """Parse a GeoJSON FeatureCollection; return (records, skipped)."""
    try:
        with open(path, "r", encoding="utf-8") as handle:
            doc = json.load(handle)
    except (OSError, ValueError) as exc:
        raise ValueError("cannot read GeoJSON %s: %s" % (path, exc))
    if doc.get("type") != "FeatureCollection":
        raise ValueError("%s is not a GeoJSON FeatureCollection" % path)

    records = []
    skipped = []
    for index, feature in enumerate(doc.get("features", [])):
        props = feature.get("properties") or {}
        lat, lon = _geometry_position(feature.get("geometry"))
        if lat is None:
            lat = parse_float(props.get("lat", props.get("latitude")))
            lon = parse_float(props.get("lon", props.get("longitude")))
        found = build_records(props, lat, lon, None,
                              "%s#%d" % (path, index), config)
        for record in found:
            if not valid_position(lat, lon):
                skipped.append(SkipRecord(
                    record.callsign, record.frequency_mhz, record.county,
                    "no_position", "feature %d has no usable position" % index))
                continue
            records.append(record)
    return records, skipped


def parse_csv(path, config):
    """Parse a CSV with a header row; return (records, skipped).

    Expected columns: callsign, lat, lon, frequency, modulation, flags,
    county.  Optional columns: elevation, height, resolved_lat, resolved_lon.
    """
    records = []
    skipped = []
    try:
        handle = open(path, "r", encoding="utf-8-sig", newline="")
    except OSError as exc:
        raise ValueError("cannot read CSV %s: %s" % (path, exc))

    with handle:
        sample = handle.read(4096)
        handle.seek(0)
        try:
            dialect = csv.Sniffer().sniff(sample, delimiters=",;\t")
        except csv.Error:
            dialect = csv.excel
        reader = csv.DictReader(handle, dialect=dialect)
        for line_no, row in enumerate(reader, start=2):
            props = {(k or "").strip().lower(): (v or "").strip()
                     for k, v in row.items()}
            lat = parse_float(props.get("lat", props.get("latitude")))
            lon = parse_float(props.get("lon", props.get("longitude")))
            found = build_records(props, lat, lon, None,
                                  "%s:%d" % (path, line_no), config)
            for record in found:
                if not valid_position(lat, lon):
                    skipped.append(SkipRecord(
                        record.callsign, record.frequency_mhz, record.county,
                        "no_position", "line %d has no usable position"
                        % line_no))
                    continue
                records.append(record)
    return records, skipped
