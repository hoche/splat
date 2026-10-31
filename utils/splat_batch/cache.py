"""Result cache: skip jobs whose inputs and parameters are unchanged."""

import hashlib
import json
import logging
import os
import shutil
import time

from .jobs import render_lcf, render_lrp, render_qth, splat_command

log = logging.getLogger(__name__)

INDEX_NAME = "cache_index.json"
_INDEX_VERSION = 1


def _file_token(path):
    """Identify an executable by resolved path, size and mtime."""
    resolved = shutil.which(path) or path
    try:
        stat = os.stat(resolved)
    except OSError:
        return "%s|missing" % resolved
    return "%s|%d|%d" % (os.path.realpath(resolved), stat.st_size,
                         stat.st_mtime_ns)


def compute_key(job, config, terrain_token):
    """SHA-256 over everything that influences a job's result.

    This covers the rendered .qth/.lrp/.lcf files (position, heights,
    frequency, climate, thresholds, colours), the complete SPLAT! command
    line, the splat binary identity, the terrain source and zoom, the
    post-processing parameters and params_version.
    """
    payload = {
        "params_version": str(config["params_version"]),
        "qth": render_qth(job),
        "lrp": render_lrp(job, config),
        "lcf": render_lcf(job, config),
        "command": splat_command(job, config),
        "splat_binary": _file_token(config["splat_bin"]),
        "terrain": terrain_token,
        "postprocess": config["postprocess"],
        "colors": config["colors"],
        "band": job.band,
        "county": job.county,
    }
    encoded = json.dumps(payload, sort_keys=True).encode("utf-8")
    return hashlib.sha256(encoded).hexdigest()


class CacheIndex:
    """JSON index mapping job id to the key of its stored result."""

    def __init__(self, out_dir):
        self.path = os.path.join(out_dir, INDEX_NAME)
        self.entries = {}
        self._load()

    def _load(self):
        if not os.path.exists(self.path):
            return
        try:
            with open(self.path, "r", encoding="utf-8") as handle:
                doc = json.load(handle)
        except (OSError, ValueError) as exc:
            log.warning("ignoring unreadable cache index %s: %s", self.path,
                        exc)
            return
        if doc.get("version") == _INDEX_VERSION:
            self.entries = doc.get("jobs", {})

    def is_fresh(self, job):
        """True if a stored result exists for job.cache_key."""
        entry = self.entries.get(job.job_id)
        return bool(entry and entry.get("key") == job.cache_key and
                    os.path.exists(job.features_file))

    def itm_error(self, job):
        """ITM error number stored with the job's cached result (0 if none)."""
        entry = self.entries.get(job.job_id) or {}
        return int(entry.get("itm_error", 0))

    def record(self, job, feature_count, itm_error=0):
        self.entries[job.job_id] = {
            "key": job.cache_key,
            "features": os.path.relpath(job.features_file,
                                        os.path.dirname(self.path)),
            "feature_count": feature_count,
            "itm_error": itm_error,
            "callsign": job.callsign,
            "frequency_mhz": job.frequency_mhz,
            "updated": time.strftime("%Y-%m-%dT%H:%M:%S%z"),
        }

    def save(self):
        doc = {"version": _INDEX_VERSION, "jobs": self.entries}
        tmp = self.path + ".tmp"
        with open(tmp, "w", encoding="utf-8") as handle:
            json.dump(doc, handle, indent=1, sort_keys=True)
        os.replace(tmp, self.path)
