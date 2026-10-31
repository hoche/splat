"""Parser for JOSM session archives (.joz).

A .joz file is a zip archive holding ``session.jos`` (layer index) and one
``layers/<name>/data.osm`` per data layer.  The layer name is taken as the
county of every repeater found in that layer.  Boundary layers (by default
any layer whose name matches "fylker") are skipped.
"""

import logging
import posixpath
import re
import xml.etree.ElementTree as ET
import zipfile

from .fields import build_records, valid_position
from .models import SkipRecord

log = logging.getLogger(__name__)


def _session_layer_names(archive):
    """Map archive member path -> layer name using session.jos if present."""
    names = {}
    if "session.jos" not in archive.namelist():
        return names
    try:
        root = ET.fromstring(archive.read("session.jos"))
    except ET.ParseError as exc:
        log.warning("session.jos is not valid XML (%s); using directory names",
                    exc)
        return names
    for layer in root.iter("layer"):
        name = layer.get("name")
        file_elem = layer.find("file")
        if name and file_elem is not None and file_elem.text:
            names[file_elem.text.strip()] = name.strip()
    return names


def _is_skipped_layer(candidates, patterns):
    for pattern in patterns:
        regex = re.compile(pattern, re.IGNORECASE)
        for candidate in candidates:
            if candidate and regex.search(candidate):
                return True
    return False


def _element_position(elem, nodes):
    """Return (lat, lon) for a node, or the mean node position for a way."""
    if elem.tag == "node":
        try:
            return float(elem.get("lat")), float(elem.get("lon"))
        except (TypeError, ValueError):
            return None, None
    coords = [nodes[nd.get("ref")] for nd in elem.findall("nd")
              if nd.get("ref") in nodes]
    if not coords:
        return None, None
    return (sum(c[0] for c in coords) / len(coords),
            sum(c[1] for c in coords) / len(coords))


def parse_osm_xml(data, county, source, config):
    """Parse the bytes of one data.osm; return a list of Repeater records."""
    root = ET.fromstring(data)
    nodes = {}
    for node in root.findall("node"):
        try:
            nodes[node.get("id")] = (float(node.get("lat")),
                                     float(node.get("lon")))
        except (TypeError, ValueError):
            continue

    records = []
    skipped = []
    for elem in list(root.findall("node")) + list(root.findall("way")):
        if elem.get("action") == "delete":
            continue
        props = {tag.get("k"): tag.get("v") for tag in elem.findall("tag")
                 if tag.get("k") is not None}
        if not props:
            continue
        lat, lon = _element_position(elem, nodes)
        found = build_records(props, lat, lon, county, source, config)
        for record in found:
            if not valid_position(lat, lon):
                skipped.append(SkipRecord(
                    record.callsign, record.frequency_mhz, county,
                    "no_position", "%s %s has no usable position"
                    % (elem.tag, elem.get("id"))))
                continue
            records.append(record)
    return records, skipped


def parse_joz(path, config):
    """Parse a .joz archive; return (records, skipped)."""
    patterns = config["skip"]["layers"]
    records = []
    skipped = []
    try:
        archive = zipfile.ZipFile(path)
    except (OSError, zipfile.BadZipFile) as exc:
        raise ValueError("cannot open JOSM session %s: %s" % (path, exc))

    with archive:
        session_names = _session_layer_names(archive)
        members = sorted(n for n in archive.namelist()
                         if re.fullmatch(r"layers/[^/]+/data\.osm", n))
        if not members:
            raise ValueError("%s contains no layers/<name>/data.osm" % path)

        for member in members:
            dirname = posixpath.basename(posixpath.dirname(member))
            layer_name = session_names.get(member, dirname)
            if _is_skipped_layer((layer_name, dirname), patterns):
                log.info("skipping layer %r (%s)", layer_name, member)
                continue
            try:
                found, bad = parse_osm_xml(archive.read(member), layer_name,
                                           "%s:%s" % (path, member), config)
            except ET.ParseError as exc:
                log.error("layer %s is not valid OSM XML: %s", member, exc)
                continue
            log.info("layer %r: %d repeater records", layer_name, len(found))
            records.extend(found)
            skipped.extend(bad)
    return records, skipped
