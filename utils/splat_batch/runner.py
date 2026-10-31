"""Execution of SPLAT! jobs, in parallel worker processes."""

import concurrent.futures
import json
import logging
import os
import re
import shutil
import subprocess
import time

from .jobs import splat_command, write_job_files
from .models import JobResult
from .postprocess import PostprocessError, postprocess_job

log = logging.getLogger(__name__)

_LOG_EXCERPT_LINES = 15

# Errors at or above this value invalidate the coverage result.
ITM_FAIL_ERROR = 4

_ITM_ERROR_RE = re.compile(
    r"(?:Longley-Rice|ITWOM)(?: model)? error number:\s*(\d+)")


def parse_itm_error(path):
    """Highest ITM/ITWOM error number found in a SPLAT! log (0 if none)."""
    try:
        with open(path, "r", encoding="utf-8", errors="replace") as handle:
            text = handle.read()
    except OSError:
        return 0
    return max((int(m) for m in _ITM_ERROR_RE.findall(text)), default=0)


def _log_excerpt(path):
    try:
        with open(path, "r", encoding="utf-8", errors="replace") as handle:
            lines = handle.read().splitlines()
    except OSError:
        return ""
    return "\n".join(lines[-_LOG_EXCERPT_LINES:])


def _write_features(job, features):
    os.makedirs(os.path.dirname(job.features_file), exist_ok=True)
    tmp = job.features_file + ".tmp"
    with open(tmp, "w", encoding="utf-8") as handle:
        json.dump({"type": "FeatureCollection", "features": features},
                  handle, separators=(",", ":"))
    os.replace(tmp, job.features_file)


def run_job(job, config):
    """Run SPLAT! for one job and post-process it.  Never raises."""
    started = time.time()
    log_file = os.path.join(job.workdir, "splat.log")
    try:
        if os.path.isdir(job.workdir):
            shutil.rmtree(job.workdir)
        write_job_files(job, config)
        cmd = splat_command(job, config)
        log.info("%s: %s", job.job_id, " ".join(cmd))

        with open(log_file, "w", encoding="utf-8") as handle:
            handle.write("$ %s\n" % " ".join(cmd))
            handle.flush()
            try:
                proc = subprocess.run(
                    cmd, cwd=job.workdir, stdout=handle,
                    stderr=subprocess.STDOUT,
                    timeout=config["splat_timeout_s"])
            except subprocess.TimeoutExpired:
                return JobResult(job.job_id, "failed",
                                 error="splat timed out after %ds"
                                 % config["splat_timeout_s"],
                                 elapsed_s=time.time() - started,
                                 log_file=log_file)
            except OSError as exc:
                return JobResult(job.job_id, "failed",
                                 error="cannot run %s: %s"
                                 % (cmd[0], exc),
                                 elapsed_s=time.time() - started,
                                 log_file=log_file)

        if proc.returncode != 0:
            return JobResult(
                job.job_id, "failed",
                error="splat exited with status %d: %s"
                % (proc.returncode, _log_excerpt(log_file)),
                elapsed_s=time.time() - started, log_file=log_file)

        itm_error = parse_itm_error(log_file)
        if itm_error >= ITM_FAIL_ERROR:
            return JobResult(
                job.job_id, "failed",
                error="itm_error_%d: SPLAT! reported ITM/ITWOM model error "
                "number %d; coverage is not reliable" % (itm_error, itm_error),
                elapsed_s=time.time() - started, log_file=log_file,
                itm_error=itm_error)

        features = postprocess_job(job, config)
        _write_features(job, features)

        if not config["keep_work"]:
            shutil.rmtree(job.workdir, ignore_errors=True)
        return JobResult(job.job_id, "ok", features_file=job.features_file,
                         feature_count=len(features),
                         elapsed_s=time.time() - started, log_file=log_file,
                         itm_error=itm_error)
    except PostprocessError as exc:
        return JobResult(job.job_id, "failed", error="postprocess: %s" % exc,
                         elapsed_s=time.time() - started, log_file=log_file)
    except Exception as exc:  # worker must report, not crash the pool
        log.exception("%s: unexpected error", job.job_id)
        return JobResult(job.job_id, "failed",
                         error="%s: %s" % (type(exc).__name__, exc),
                         elapsed_s=time.time() - started, log_file=log_file)


def run_jobs(jobs, config, workers, on_result):
    """Run jobs in a process pool; call on_result(job, result) for each."""
    if not jobs:
        return
    pool = concurrent.futures.ProcessPoolExecutor(max_workers=workers)
    try:
        futures = {pool.submit(run_job, job, config): job for job in jobs}
        for future in concurrent.futures.as_completed(futures):
            job = futures[future]
            try:
                result = future.result()
            except Exception as exc:  # e.g. worker killed by the OS
                result = JobResult(job.job_id, "failed",
                                   error="worker failed: %s: %s"
                                   % (type(exc).__name__, exc))
            on_result(job, result)
    except KeyboardInterrupt:
        log.error("interrupted; cancelling pending jobs")
        pool.shutdown(wait=False, cancel_futures=True)
        raise
    else:
        pool.shutdown(wait=True)
