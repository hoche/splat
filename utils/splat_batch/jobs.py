"""Job construction and generation of the per-job SPLAT! input files."""

import logging
import math
import os
import re
import hashlib

from .filters import band_for, frequency_key
from .models import Job

log = logging.getLogger(__name__)

KM_PER_MILE = 1.609344

# SPLAT! (see main.cpp): analysis half-range in degrees is
# miles / 57, limited by a table keyed on -maxpages.
_SPLAT_MILES_PER_DEGREE = 57.0
_MAXPAGES_DEG_LIMIT = (
    (1, 0.125), (4, 0.5), (9, 1.0), (16, 1.5),
    (25, 2.0), (36, 2.5), (49, 3.0), (64, 3.5),
)

# Names of the files written inside each job directory.  A fixed basename
# avoids SPLAT! deriving .lcf/.lrp names from callsigns containing dots.
SITE_BASENAME = "site"


def splat_ranges_deg(lat, radius_km, maxpages=None):
    """Return the (lat, lon) half-range in degrees that SPLAT! analyses.

    If maxpages is given, the SPLAT! per-maxpages cap is applied.
    """
    deg_range = radius_km / KM_PER_MILE / _SPLAT_MILES_PER_DEGREE
    cos_lat = math.cos(math.radians(min(abs(lat), 70.0)))
    deg_range_lon = deg_range / cos_lat
    if maxpages is not None:
        limit = dict(_MAXPAGES_DEG_LIMIT)[maxpages]
        deg_range = min(deg_range, limit)
        deg_range_lon = min(deg_range_lon, limit)
    return deg_range, deg_range_lon


def required_maxpages(lat, radius_km):
    """Smallest SPLAT! -maxpages value whose range cap covers the radius."""
    deg_range, deg_range_lon = splat_ranges_deg(lat, radius_km)
    needed = max(deg_range, deg_range_lon)
    for pages, limit in _MAXPAGES_DEG_LIMIT:
        if limit >= needed:
            return pages
    log.warning("radius %.0f km at latitude %.2f exceeds the largest SPLAT! "
                "range cap; coverage will be truncated", radius_km, lat)
    return _MAXPAGES_DEG_LIMIT[-1][0]


def job_bbox(job, margin_deg=0.0):
    """Return (min_lat, min_lon, max_lat, max_lon) in degrees east/north."""
    lat_range, lon_range = splat_ranges_deg(job.lat, job.radius_km,
                                            job.maxpages or None)
    lat_range += margin_deg
    lon_range += margin_deg
    return (max(job.lat - lat_range, -85.0), job.lon - lon_range,
            min(job.lat + lat_range, 85.0), job.lon + lon_range)


def _safe_name(text):
    return re.sub(r"[^A-Za-z0-9_-]", "_", text)


def _select_radius(rep, tx_height, config, radius_km):
    high = (
        rep.callsign in {c.upper() for c in config["high_site_callsigns"]} or
        (rep.elevation_m is not None and
         rep.elevation_m >= config["high_site_elevation_m"]) or
        tx_height >= config["high_site_height_m"])
    if high:
        return max(config["radius_high_km"], radius_km)
    return radius_km


def _select_climate(rep, config):
    climates = config["climates"]
    if rep.callsign in climates["by_callsign"]:
        return int(climates["by_callsign"][rep.callsign])
    if rep.county in climates["by_county"]:
        return int(climates["by_county"][rep.county])
    return int(climates["default"])


def build_jobs(repeaters, config, out_dir):
    """Create one Job per repeater with directories rooted in out_dir."""
    jobs = []
    used_ids = set()
    for rep in repeaters:
        band = band_for(rep.frequency_mhz, config)
        tx_height = (rep.height_m if rep.height_m is not None
                     else config["tx_height_m"])
        radius = _select_radius(rep, tx_height, config, config["radius_km"])
        job_id = "%s_%d" % (_safe_name(rep.callsign),
                            frequency_key(rep.frequency_mhz))
        if job_id in used_ids:
            digest = hashlib.sha1(rep.callsign.encode("utf-8")).hexdigest()
            job_id = "%s_%s" % (job_id, digest[:6])
        used_ids.add(job_id)

        if config["auto_maxpages"]:
            maxpages = required_maxpages(rep.lat, radius)
        else:
            maxpages = 0

        jobs.append(Job(
            job_id=job_id,
            callsign=rep.callsign,
            county=rep.county or "unknown",
            lat=rep.lat,
            lon=rep.lon,
            frequency_mhz=rep.frequency_mhz,
            band=band["name"],
            hear_db=float(band["hear_db"]),
            talk_db=float(band["talk_db"]),
            tx_height_m=float(tx_height),
            rx_height_m=float(config["rx_height_m"]),
            radius_km=float(radius),
            climate=_select_climate(rep, config),
            maxpages=maxpages,
            workdir=os.path.join(out_dir, "work", job_id),
            features_file=os.path.join(out_dir, "features", job_id + ".geojson"),
        ))
    return jobs


# ---------------------------------------------------------------------------
# Per-job input files
# ---------------------------------------------------------------------------

def render_qth(job):
    """Contents of the .qth file.  SPLAT! longitudes are degrees west."""
    west = (-job.lon) % 360.0
    return "%s\n%.6f\n%.6f\n%gm\n" % (job.callsign, job.lat, west,
                                      job.tx_height_m)


def render_lrp(job, config):
    """Contents of the .lrp file (ERP 0 selects path-loss output)."""
    lrp = config["lrp"]
    return (
        "%.3f  ; Earth dielectric constant\n"
        "%.3f  ; Earth conductivity (S/m)\n"
        "%.3f  ; Atmospheric bending constant (N-units)\n"
        "%.3f  ; Frequency (MHz)\n"
        "%d  ; Radio climate\n"
        "%d  ; Polarisation (1 = vertical)\n"
        "%.2f  ; Fraction of situations\n"
        "%.2f  ; Fraction of time\n"
        "%g  ; ERP (0 selects path-loss output)\n"
    ) % (lrp["dielectric"], lrp["conductivity"], lrp["bending"],
         job.frequency_mhz, job.climate, lrp["polarization"],
         lrp["fraction_situations"], lrp["fraction_time"], lrp["erp"])


def lcf_levels(job):
    """Return the integer LCF thresholds (talk, hear_upper).

    SPLAT! path-loss pixels are integer dB.  The first LCF level colours
    loss <= level[0]; the second colours level[0] < loss < level[1].  To
    make "hear" inclusive of its own threshold, the second level is the
    hear threshold plus one.
    """
    talk = int(math.floor(job.talk_db))
    hear = int(math.floor(job.hear_db)) + 1
    return talk, hear


def render_lcf(job, config):
    """Contents of the .lcf file colouring the talk and hear regions."""
    talk, hear = lcf_levels(job)
    talk_rgb = config["colors"]["talk"]
    hear_rgb = config["colors"]["hear"]
    return (
        "; Generated by splat-batch for %s\n"
        "; talk: path loss <= %g dB, hear: path loss <= %g dB\n"
        "%3d: %3d, %3d, %3d\n"
        "%3d: %3d, %3d, %3d\n"
    ) % (job.callsign, job.talk_db, job.hear_db,
         talk, talk_rgb[0], talk_rgb[1], talk_rgb[2],
         hear, hear_rgb[0], hear_rgb[1], hear_rgb[2])


def terrain_args(config):
    """SPLAT! command-line arguments selecting the Mapterhorn terrain."""
    mt = config["mapterhorn"]
    if not mt["enabled"]:
        return []
    subst = {"source": mt["source"], "cache": mt["cache"],
             "zoom": mt.get("zoom")}
    args = [a.format(**subst) for a in mt["splat_args"]]
    if mt.get("zoom") is not None:
        args += [a.format(**subst) for a in mt["zoom_args"]]
    return args


def splat_command(job, config):
    """Full SPLAT! command line, with job-relative file names."""
    # -st: single-threaded SPLAT!.  The batch already parallelises over
    # jobs, and multi-threaded path-loss runs are not bit-reproducible,
    # which would make cached results unstable.
    cmd = [config["splat_bin"], "-st"]
    if config["hd"]:
        cmd.append("-hd")
    cmd += ["-t", SITE_BASENAME + ".qth",
            "-L", "%g" % job.rx_height_m,
            "-R", "%g" % job.radius_km]
    extra = list(config["splat_extra_args"])
    if job.maxpages and "-maxpages" not in extra:
        cmd += ["-maxpages", str(job.maxpages)]
    cmd += terrain_args(config)
    cmd += extra
    cmd += ["-o", SITE_BASENAME, "-kml", "-ppm"]
    return cmd


def write_job_files(job, config):
    """Create the job directory and write its .qth, .lrp and .lcf files."""
    os.makedirs(job.workdir, exist_ok=True)
    contents = {
        SITE_BASENAME + ".qth": render_qth(job),
        SITE_BASENAME + ".lrp": render_lrp(job, config),
        SITE_BASENAME + ".lcf": render_lcf(job, config),
    }
    for name, text in contents.items():
        with open(os.path.join(job.workdir, name), "w",
                  encoding="utf-8") as handle:
            handle.write(text)
