"""Command-line interface and run orchestration for splat-batch."""

import argparse
import logging
import os
import shlex
import sys
import time

from . import __version__
from .cache import CacheIndex, compute_key
from .config import ConfigError, apply_overrides, load_config, validate
from .deps import check_dependencies
from .filters import select
from .inputs import FORMATS, read_inputs
from .jobs import build_jobs
from .output import OutputError, write_county_outputs
from .report import RunReport
from .runner import run_jobs
from .terrain import PrefetchError, prefetch, terrain_token

log = logging.getLogger("splat_batch")


def build_parser():
    parser = argparse.ArgumentParser(
        prog="splat-batch",
        description="Run one independent SPLAT! coverage computation per "
                    "repeater and write hear/talk polygons per county.")
    parser.add_argument("--version", action="version",
                        version="splat-batch " + __version__)

    io_group = parser.add_argument_group("input and output")
    io_group.add_argument("--input", "-i", action="append", required=True,
                          metavar="FILE",
                          help="repeater list (.joz, .geojson, .csv); "
                               "may be given more than once")
    io_group.add_argument("--input-format", choices=FORMATS,
                          help="override format detection by extension")
    io_group.add_argument("--out-dir", "-o", required=True, metavar="DIR",
                          help="output directory (also holds the cache)")
    io_group.add_argument("--config", "-c", metavar="FILE",
                          help="JSON config merged over the packaged defaults")
    io_group.add_argument("--formats", metavar="LIST",
                          help="comma-separated: geojson,flatgeobuf")

    splat_group = parser.add_argument_group("SPLAT! execution")
    splat_group.add_argument("--splat-bin", metavar="PATH")
    splat_group.add_argument("--hd", dest="hd", action="store_true",
                             default=None, help="high definition mode (-hd)")
    splat_group.add_argument("--no-hd", dest="hd", action="store_false",
                             help="standard definition mode")
    splat_group.add_argument("--splat-args", metavar="STRING",
                             help="extra SPLAT! arguments, shell-quoted "
                                  "(for example \"-d /data/sdf\")")
    splat_group.add_argument("--workers", "-j", type=int,
                             default=max(1, (os.cpu_count() or 2) // 2),
                             help="parallel SPLAT! jobs (default: %(default)s)")
    splat_group.add_argument("--radius-km", type=float)
    splat_group.add_argument("--radius-high-km", type=float,
                             help="radius for high sites")
    splat_group.add_argument("--tx-height", type=float, metavar="M",
                             help="transmitter height AGL in metres")
    splat_group.add_argument("--rx-height", type=float, metavar="M",
                             help="receiver height AGL in metres")
    splat_group.add_argument("--climate", type=int,
                             help="default radio climate code")

    terrain_group = parser.add_argument_group("terrain (Mapterhorn)")
    terrain_group.add_argument("--mapterhorn", action="store_true",
                               default=None,
                               help="use Mapterhorn terrain and prefetch it")
    terrain_group.add_argument("--mt-source", metavar="PMTILES")
    terrain_group.add_argument("--mt-cache", metavar="DIR")
    terrain_group.add_argument("--mt-zoom", type=int)
    terrain_group.add_argument("--mapterhorn2sdf", metavar="PATH",
                               help="mapterhorn2sdf executable")
    terrain_group.add_argument("--prefetch-workers", type=int, default=2,
                               help="workers for mapterhorn2sdf "
                                    "(default: %(default)s)")

    post_group = parser.add_argument_group("filters and post-processing")
    post_group.add_argument("--simplify-m", type=float, metavar="M",
                            help="inward simplification tolerance in metres")
    post_group.add_argument("--min-area-ha", type=float,
                            help="drop islands smaller than this")
    post_group.add_argument("--skip-flag", action="append", metavar="TEXT",
                            help="additional flag text that excludes a "
                                 "repeater (case-insensitive)")
    post_group.add_argument("--include-data", action="store_true",
                            help="do not skip data-only modulations")
    post_group.add_argument("--only", action="append", metavar="CALLSIGN",
                            help="restrict the run to these callsigns")
    post_group.add_argument("--params-version", metavar="STRING")

    run_group = parser.add_argument_group("run control")
    run_group.add_argument("--force", action="store_true",
                           help="ignore the cache and rerun every job")
    run_group.add_argument("--keep-work", action="store_true",
                           help="keep per-job working directories")
    run_group.add_argument("--dry-run", action="store_true",
                           help="parse and filter only; list the jobs")
    run_group.add_argument("--verbose", "-v", action="store_true")
    return parser


def _config_from_args(args):
    config = load_config(args.config)
    skip = config["skip"]
    if args.skip_flag:
        skip["flags"] = list(skip["flags"]) + list(args.skip_flag)
    overrides = {
        "splat_bin": args.splat_bin,
        "hd": args.hd,
        "radius_km": args.radius_km,
        "radius_high_km": args.radius_high_km,
        "tx_height_m": args.tx_height,
        "rx_height_m": args.rx_height,
        "params_version": args.params_version,
        "keep_work": True if args.keep_work else None,
        "climates": {"default": args.climate},
        "skip": {"skip_data_only": False if args.include_data else None},
        "mapterhorn": {
            "enabled": args.mapterhorn,
            "source": args.mt_source,
            "cache": args.mt_cache,
            "zoom": args.mt_zoom,
            "bin": args.mapterhorn2sdf,
        },
        "postprocess": {
            "simplify_tolerance_m": args.simplify_m,
            "min_area_ha": args.min_area_ha,
        },
    }
    apply_overrides(config, overrides)
    if args.splat_args:
        config["splat_extra_args"] = (list(config["splat_extra_args"]) +
                                      shlex.split(args.splat_args))
    if args.formats:
        config["output"]["formats"] = [f.strip() for f in
                                       args.formats.split(",") if f.strip()]
    return validate(config)


def _setup_logging(verbose, log_path=None):
    level = logging.DEBUG if verbose else logging.INFO
    handlers = [logging.StreamHandler(sys.stderr)]
    if log_path:
        handlers.append(logging.FileHandler(log_path, encoding="utf-8"))
    logging.basicConfig(level=level, handlers=handlers, force=True,
                        format="%(asctime)s %(levelname)s %(message)s",
                        datefmt="%H:%M:%S")


def _print_plan(jobs, pending, skipped):
    pending_ids = {job.job_id for job in pending}
    for job in jobs:
        state = "run" if job.job_id in pending_ids else "cached"
        print("%-6s %-24s %9.4f MHz %-5s %-12s r=%gkm climate=%d (%.5f, %.5f)"
              % (state, job.job_id, job.frequency_mhz, job.band, job.county,
                 job.radius_km, job.climate, job.lat, job.lon))
    for rec in skipped:
        print("skip   %-14s %-12s %s %s" % (rec.callsign, rec.reason,
                                            rec.county, rec.detail))


def main(argv=None):
    args = build_parser().parse_args(argv)
    started = time.time()

    try:
        config = _config_from_args(args)
    except ConfigError as exc:
        print("splat-batch: configuration error: %s" % exc, file=sys.stderr)
        return 2

    if not args.dry_run:
        problems = check_dependencies(config)
        if problems:
            print("splat-batch: cannot start:", file=sys.stderr)
            for problem in problems:
                print("  - " + problem, file=sys.stderr)
            return 2

    out_dir = os.path.abspath(args.out_dir)
    if args.dry_run:
        _setup_logging(args.verbose)
    else:
        os.makedirs(out_dir, exist_ok=True)
        _setup_logging(args.verbose, os.path.join(out_dir, "splat-batch.log"))

    report = RunReport()
    try:
        records, bad = read_inputs(args.input, config, args.input_format)
    except ValueError as exc:
        log.error("%s", exc)
        return 2
    report.add_skipped(bad)

    repeaters, skipped = select(records, config)
    report.add_skipped(skipped)
    if args.only:
        wanted = {c.upper() for c in args.only}
        repeaters = [r for r in repeaters if r.callsign in wanted]
    log.info("%d input records, %d jobs after filtering, %d skipped",
             len(records), len(repeaters), len(report.skipped))

    jobs = build_jobs(repeaters, config, out_dir)
    token = terrain_token(config)
    for job in jobs:
        job.cache_key = compute_key(job, config, token)

    index = CacheIndex(out_dir)
    pending = [j for j in jobs if args.force or not index.is_fresh(j)]
    pending_ids = {j.job_id for j in pending}
    cached = [j for j in jobs if j.job_id not in pending_ids]

    if args.dry_run:
        _print_plan(jobs, pending, report.skipped)
        return 0

    for job in cached:
        report.add_cached(job.job_id, index.itm_error(job))
    log.info("%d job(s) to run, %d unchanged (cached)", len(pending),
             len(cached))

    if config["mapterhorn"]["enabled"] and pending:
        try:
            prefetch(pending, config, args.prefetch_workers)
        except PrefetchError as exc:
            log.error("terrain prefetch failed: %s", exc)
            return 1

    completed = list(cached)

    def on_result(job, result):
        if result.status == "ok":
            index.record(job, result.feature_count, result.itm_error)
            index.save()
            completed.append(job)
            report.add_run(job.job_id, result.itm_error)
            log.info("%s: done in %.0f s, %d feature(s)", job.job_id,
                     result.elapsed_s, result.feature_count)
            if result.itm_error == 3:
                log.warning("%s: ITM model error number 3 (result kept, "
                            "flagged in the report)", job.job_id)
        else:
            report.add_failed(job.job_id, result.error, result.log_file,
                              result.itm_error)
            log.error("%s: FAILED: %s", job.job_id, result.error)

    try:
        run_jobs(pending, config, args.workers, on_result)
    except KeyboardInterrupt:
        index.save()
        return 130

    try:
        report.outputs = write_county_outputs(completed, out_dir, config)
    except OutputError as exc:
        log.error("%s", exc)
        return 1

    report.elapsed_s = time.time() - started
    report.write_json(os.path.join(out_dir, "report.json"))
    report.write_attribution(os.path.join(out_dir, "ATTRIBUTION.txt"))
    print(report.format_text())
    return 1 if report.failed else 0


if __name__ == "__main__":
    sys.exit(main())
