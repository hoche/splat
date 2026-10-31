"""Helpers shared by the input parsers: tag lookup and value normalisation."""

import re

from .models import Repeater

_NUMBER = re.compile(r"[-+]?\d+(?:[.,]\d+)?")


def first_value(props, keys):
    """Return the first non-empty string value among keys, else None."""
    for key in keys:
        value = props.get(key)
        if value is None:
            continue
        text = str(value).strip()
        if text:
            return text
    return None


def parse_float(text):
    """Parse a float from text that may carry units or a decimal comma."""
    if text is None:
        return None
    match = _NUMBER.search(str(text))
    if not match:
        return None
    try:
        return float(match.group(0).replace(",", "."))
    except ValueError:
        return None


def normalise_mhz(value):
    """Interpret a bare number as MHz, kHz or Hz depending on magnitude."""
    if value >= 1.0e6:
        return value / 1.0e6
    if value >= 1.0e4:
        return value / 1.0e3
    return value


def parse_frequencies(text):
    """Return all frequencies (MHz) found in a field.

    Values may be separated by semicolons and may carry a unit suffix
    such as " MHz".  Bare numbers of 10000 and above are treated as kHz,
    1e6 and above as Hz.
    """
    if text is None:
        return []
    result = []
    for part in re.split(r"[;|]", str(text)):
        part = part.strip()
        if not part:
            continue
        value = parse_float(part)
        if value is None or value <= 0:
            continue
        lowered = part.lower()
        if "khz" in lowered:
            value /= 1.0e3
        elif "ghz" in lowered:
            value *= 1.0e3
        elif "mhz" not in lowered:
            value = normalise_mhz(value)
        result.append(round(value, 6))
    return result


def build_records(props, lat, lon, county, source, config):
    """Create one Repeater per output frequency from a flat property map.

    Returns an empty list when the element carries neither a callsign nor a
    frequency, i.e. it is not a repeater element at all.
    """
    tags = config["tags"]
    callsign = first_value(props, tags["callsign"])
    freq_text = first_value(props, tags["frequency"])
    if callsign is None and freq_text is None:
        return []

    freqs = parse_frequencies(freq_text)
    if not freqs:
        freqs = [None]

    if county is None or not str(county).strip():
        county = first_value(props, tags["county"]) or ""

    records = []
    for freq in freqs:
        records.append(Repeater(
            callsign=(callsign or "").strip().upper(),
            lat=lat,
            lon=lon,
            frequency_mhz=freq,
            modulation=first_value(props, tags["modulation"]) or "",
            flags=first_value(props, tags["flags"]) or "",
            county=str(county).strip(),
            source=source,
            elevation_m=parse_float(first_value(props, tags["elevation"])),
            height_m=parse_float(first_value(props, tags["height"])),
            resolved_lat=parse_float(first_value(props, tags["resolved_lat"])),
            resolved_lon=parse_float(first_value(props, tags["resolved_lon"])),
        ))
    return records


def valid_position(lat, lon):
    return (lat is not None and lon is not None and
            -90.0 <= lat <= 90.0 and -180.0 <= lon <= 360.0)
