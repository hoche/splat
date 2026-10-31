"""Record filtering and per-(callsign, frequency) position resolution."""

import logging
import math
from collections import OrderedDict, defaultdict

from .models import Repeater, SkipRecord

log = logging.getLogger(__name__)

_METERS_PER_DEGREE = 111320.0


def band_for(frequency_mhz, config):
    """Return the band definition containing frequency_mhz, or None."""
    for band in config["bands"]:
        if band["min_mhz"] <= frequency_mhz <= band["max_mhz"]:
            return band
    return None


def _modulation_tokens(modulation):
    tokens = []
    for part in modulation.replace(",", ";").replace("/", ";").split(";"):
        for token in part.split():
            tokens.append(token.strip().upper())
    return [t for t in tokens if t]


def is_data_only(modulation, config):
    """True if modulation lists only data modes from the configured set."""
    tokens = _modulation_tokens(modulation or "")
    if not tokens:
        return False
    data_modes = {m.upper() for m in config["skip"]["data_only_modulations"]}
    return all(token in data_modes for token in tokens)


def has_flag(flags, needle):
    return needle.lower() in (flags or "").lower()


def frequency_key(frequency_mhz):
    return int(round(frequency_mhz * 1000.0))


def _distance_m(a, b):
    mean_lat = math.radians((a[0] + b[0]) / 2.0)
    dy = (a[0] - b[0]) * _METERS_PER_DEGREE
    dx = (a[1] - b[1]) * _METERS_PER_DEGREE * math.cos(mean_lat)
    return math.hypot(dx, dy)


def _cluster_positions(records, tolerance_m):
    """Group records whose positions lie within tolerance_m of a cluster seed."""
    clusters = []
    for record in records:
        pos = (record.lat, record.lon)
        for cluster in clusters:
            if _distance_m(pos, (cluster[0].lat, cluster[0].lon)) <= tolerance_m:
                cluster.append(record)
                break
        else:
            clusters.append([record])
    return clusters


def _simple_filter(record, config):
    """Return a skip reason and detail for one record, or None to keep it."""
    skip = config["skip"]
    if not record.callsign:
        return "missing_callsign", "record has a frequency but no callsign"
    if record.frequency_mhz is None:
        return "missing_frequency", "record has no parsable frequency"
    for flag in skip["flags"]:
        if has_flag(record.flags, flag):
            return "flag_" + flag.lower(), "flags=%r" % record.flags
    if skip.get("skip_data_only", True) and is_data_only(record.modulation,
                                                         config):
        return "data_only_modulation", "modulation=%r" % record.modulation
    if band_for(record.frequency_mhz, config) is None:
        return "unsupported_band", "no band configured for %.4f MHz" % (
            record.frequency_mhz)
    return None


def _resolve_group(group, config):
    """Reduce all records of one (callsign, frequency) to one Repeater.

    Returns (repeater, None) on success or (None, SkipRecord) on failure.
    """
    skip = config["skip"]
    first = group[0]
    key_text = "%s %.4f MHz" % (first.callsign, first.frequency_mhz)

    def fail(reason, detail):
        return None, SkipRecord(first.callsign, first.frequency_mhz,
                                first.county, reason, detail)

    resolved = sorted({(r.resolved_lat, r.resolved_lon)
                       for r in group if r.has_resolved_position})
    clusters = _cluster_positions(group, skip["position_tolerance_m"])
    flagged = any(has_flag(r.flags, skip["disagreement_flag"]) for r in group)

    if len(resolved) > 1:
        log.warning("%s: conflicting resolved positions %s; skipped",
                    key_text, resolved)
        return fail("position_disagreement",
                    "conflicting resolved positions: %s" % resolved)

    if len(resolved) == 1:
        lat, lon = resolved[0]
        if len(clusters) > 1:
            log.info("%s: %d positions in input, using resolved position",
                     key_text, len(clusters))
    elif len(clusters) == 1:
        if flagged and not skip["allow_single_position_disagreement"]:
            log.warning("%s: flagged %s; skipped", key_text,
                        skip["disagreement_flag"])
            return fail("disagreement", "flags=%r" % first.flags)
        members = clusters[0]
        lat = sum(r.lat for r in members) / len(members)
        lon = sum(r.lon for r in members) / len(members)
        if flagged:
            log.info("%s: flagged %s but a single position exists; kept",
                     key_text, skip["disagreement_flag"])
    else:
        positions = ["%.5f,%.5f (%s)" % (c[0].lat, c[0].lon, c[0].county)
                     for c in clusters]
        log.warning("%s: %d disagreeing positions without a resolved "
                    "position: %s; skipped", key_text, len(clusters),
                    "; ".join(positions))
        return fail("position_disagreement",
                    "%d positions: %s" % (len(clusters), "; ".join(positions)))

    def first_set(attr):
        for record in group:
            value = getattr(record, attr)
            if value not in (None, ""):
                return value
        return None

    return Repeater(
        callsign=first.callsign,
        lat=lat,
        lon=lon,
        frequency_mhz=first.frequency_mhz,
        modulation=first_set("modulation") or "",
        flags=first_set("flags") or "",
        county=first_set("county") or "",
        source=first.source,
        elevation_m=first_set("elevation_m"),
        height_m=first_set("height_m"),
    ), None


def _log_frequency_conflicts(repeaters):
    by_call = defaultdict(set)
    for rep in repeaters:
        by_call[rep.callsign].add(rep.frequency_mhz)
    for callsign, freqs in sorted(by_call.items()):
        if len(freqs) > 1:
            log.warning("callsign %s appears with %d output frequencies "
                        "(%s); one job is created per frequency", callsign,
                        len(freqs),
                        ", ".join("%.4f" % f for f in sorted(freqs)))


def select(records, config):
    """Apply all filters; return (repeaters, skipped).

    One Repeater is returned per (callsign, output frequency).
    """
    skipped = []
    groups = OrderedDict()
    for record in records:
        verdict = _simple_filter(record, config)
        if verdict is not None:
            reason, detail = verdict
            skipped.append(SkipRecord(record.callsign, record.frequency_mhz,
                                      record.county, reason, detail))
            continue
        key = (record.callsign, frequency_key(record.frequency_mhz))
        groups.setdefault(key, []).append(record)

    repeaters = []
    for group in groups.values():
        repeater, skip_record = _resolve_group(group, config)
        if skip_record is not None:
            skipped.append(skip_record)
        else:
            repeaters.append(repeater)

    _log_frequency_conflicts(repeaters)
    return repeaters, skipped
