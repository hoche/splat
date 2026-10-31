"""Input format detection and dispatch."""

import os

from .parse_joz import parse_joz
from .parse_tabular import parse_csv, parse_geojson

_BY_EXTENSION = {
    ".joz": "joz",
    ".geojson": "geojson",
    ".json": "geojson",
    ".csv": "csv",
}

_PARSERS = {
    "joz": parse_joz,
    "geojson": parse_geojson,
    "csv": parse_csv,
}

FORMATS = tuple(sorted(_PARSERS))


def detect_format(path):
    ext = os.path.splitext(path)[1].lower()
    fmt = _BY_EXTENSION.get(ext)
    if fmt is None:
        raise ValueError("cannot infer input format of %s; use --input-format"
                         % path)
    return fmt


def read_inputs(paths, config, forced_format=None):
    """Parse all input files; return (records, skipped)."""
    records = []
    skipped = []
    for path in paths:
        fmt = forced_format or detect_format(path)
        found, bad = _PARSERS[fmt](path, config)
        records.extend(found)
        skipped.extend(bad)
    return records, skipped
