"""Configuration loading, merging and validation."""

import copy
import json
import os

DEFAULT_CONFIG_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                   "config_default.json")


class ConfigError(Exception):
    """Raised for an invalid configuration."""


def _deep_merge(base, override):
    """Return base updated recursively with override (lists are replaced)."""
    result = copy.deepcopy(base)
    for key, value in override.items():
        if isinstance(value, dict) and isinstance(result.get(key), dict):
            result[key] = _deep_merge(result[key], value)
        else:
            result[key] = copy.deepcopy(value)
    return result


def _read_json(path):
    try:
        with open(path, "r", encoding="utf-8") as handle:
            return json.load(handle)
    except OSError as exc:
        raise ConfigError("cannot read config %s: %s" % (path, exc))
    except ValueError as exc:
        raise ConfigError("invalid JSON in %s: %s" % (path, exc))


def load_config(user_path=None):
    """Load the packaged default config, merged with an optional user file."""
    config = _read_json(DEFAULT_CONFIG_PATH)
    if user_path:
        user = _read_json(user_path)
        if not isinstance(user, dict):
            raise ConfigError("config %s must contain a JSON object" % user_path)
        config = _deep_merge(config, user)
    return config


def apply_overrides(config, overrides):
    """Apply a nested dict of overrides, ignoring entries whose value is None."""
    for key, value in overrides.items():
        if value is None:
            continue
        if isinstance(value, dict):
            sub = config.setdefault(key, {})
            apply_overrides(sub, value)
        else:
            config[key] = value
    return config


def validate(config):
    """Check the merged configuration; raise ConfigError on problems."""
    if not str(config.get("params_version", "")).strip():
        raise ConfigError("params_version must be a non-empty string")

    for key in ("tx_height_m", "rx_height_m", "radius_km", "radius_high_km"):
        value = config.get(key)
        if not isinstance(value, (int, float)) or value <= 0:
            raise ConfigError("%s must be a positive number" % key)

    colors = config["colors"]
    for level in ("talk", "hear"):
        rgb = colors.get(level)
        if (not isinstance(rgb, list) or len(rgb) != 3 or
                any(not isinstance(c, int) or c < 0 or c > 255 for c in rgb)):
            raise ConfigError("colors.%s must be [r, g, b] with 0..255" % level)
        if rgb[0] == rgb[1] == rgb[2]:
            raise ConfigError(
                "colors.%s must not be a grey value; SPLAT! paints terrain "
                "in grey and the masks are recovered by exact colour match"
                % level)
    if colors["talk"] == colors["hear"]:
        raise ConfigError("colors.talk and colors.hear must differ")

    for band in config["bands"]:
        for key in ("name", "min_mhz", "max_mhz", "hear_db", "talk_db"):
            if key not in band:
                raise ConfigError("band entry %r lacks %r" % (band, key))
        if band["min_mhz"] >= band["max_mhz"]:
            raise ConfigError("band %s: min_mhz must be below max_mhz"
                              % band["name"])
        if band["talk_db"] >= band["hear_db"]:
            raise ConfigError("band %s: talk_db must be below hear_db"
                              % band["name"])

    post = config["postprocess"]
    if post["simplify_tolerance_m"] < 0 or post["min_area_ha"] < 0:
        raise ConfigError("postprocess values must not be negative")

    formats = config["output"]["formats"]
    for fmt in formats:
        if fmt not in ("geojson", "flatgeobuf"):
            raise ConfigError("unknown output format %r" % fmt)
    if not formats:
        raise ConfigError("output.formats must not be empty")

    mt = config["mapterhorn"]
    if mt["enabled"] and not (mt.get("source") and mt.get("cache")):
        raise ConfigError("mapterhorn requires both a source and a cache "
                          "directory (--mt-source / --mt-cache)")
    return config
