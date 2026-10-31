"""Run report: counts, skip reasons, failures and output size."""

import json
from collections import Counter

# ITM error numbers (see runner.ITM_FAIL_ERROR for the failure threshold).
_ITM_WARN_ERROR = 3
_ITM_FAIL_ERROR = 4


class RunReport:
    """Accumulates the outcome of one splat-batch run."""

    def __init__(self):
        self.run = []        # job ids executed successfully
        self.cached = []     # job ids reused from the cache
        self.failed = []     # dicts: job_id, error, log_file, itm_error
        self.skipped = []    # SkipRecord instances
        self.outputs = []    # (path, size_bytes)
        self.itm_error_3 = []  # ok or cached job ids with ITM error 3
        self.elapsed_s = 0.0

    def add_run(self, job_id, itm_error=0):
        self.run.append(job_id)
        self._note_itm(job_id, itm_error)

    def add_cached(self, job_id, itm_error=0):
        self.cached.append(job_id)
        self._note_itm(job_id, itm_error)

    def add_failed(self, job_id, error, log_file="", itm_error=0):
        self.failed.append({"job_id": job_id, "error": error,
                            "log_file": log_file, "itm_error": itm_error})

    def add_skipped(self, records):
        self.skipped.extend(records)

    def _note_itm(self, job_id, itm_error):
        if itm_error == _ITM_WARN_ERROR:
            self.itm_error_3.append(job_id)

    @property
    def itm_error_ge_4(self):
        """Number of jobs failed because of an ITM error number >= 4."""
        return sum(1 for item in self.failed
                   if item.get("itm_error", 0) >= _ITM_FAIL_ERROR)

    @property
    def total_output_bytes(self):
        return sum(size for _, size in self.outputs)

    def skip_reasons(self):
        return Counter(rec.reason for rec in self.skipped)

    def as_dict(self):
        return {
            "attribution": {
                "terrain": "Mapterhorn",
                "url": "https://mapterhorn.com/attribution",
                "note": "Required whenever these coverage polygons are "
                        "redistributed or shown in an application.",
            },
            "jobs_run": len(self.run),
            "jobs_cached": len(self.cached),
            "jobs_failed": len(self.failed),
            "jobs_skipped": len(self.skipped),
            "itm_error_3": len(self.itm_error_3),
            "itm_error_ge_4": self.itm_error_ge_4,
            "itm_error_3_jobs": sorted(self.itm_error_3),
            "skip_reasons": dict(sorted(self.skip_reasons().items())),
            "run": sorted(self.run),
            "cached": sorted(self.cached),
            "failed": self.failed,
            "skipped": [rec.as_dict() for rec in self.skipped],
            "outputs": [{"path": p, "bytes": s} for p, s in self.outputs],
            "total_output_bytes": self.total_output_bytes,
            "elapsed_s": round(self.elapsed_s, 1),
        }

    def write_json(self, path):
        with open(path, "w", encoding="utf-8") as handle:
            json.dump(self.as_dict(), handle, indent=1)

    def write_attribution(self, path):
        """Write a standalone attribution file next to the county outputs."""
        text = (
            "Terrain data: Mapterhorn\n"
            "Attribution (required): https://mapterhorn.com/attribution\n"
            "\n"
            "Include this credit wherever these coverage polygons are\n"
            "redistributed, packaged, or shown in an application.\n"
        )
        with open(path, "w", encoding="utf-8") as handle:
            handle.write(text)

    def format_text(self):
        lines = [
            "splat-batch run report",
            "  attribution         : Mapterhorn "
            "(https://mapterhorn.com/attribution)",
            "  jobs run (new)      : %d" % len(self.run),
            "  jobs cached         : %d" % len(self.cached),
            "  jobs failed         : %d" % len(self.failed),
            "  itm error 3 (ok)    : %d" % len(self.itm_error_3),
            "  itm error >= 4      : %d" % self.itm_error_ge_4,
            "  entries skipped     : %d" % len(self.skipped),
        ]
        for reason, count in sorted(self.skip_reasons().items()):
            lines.append("    %-24s: %d" % (reason, count))
        for job_id in sorted(self.itm_error_3):
            lines.append("  ITM ERROR 3 %s" % job_id)
        for item in self.failed:
            first_line = item["error"].splitlines()[0] if item["error"] else ""
            lines.append("  FAILED %s: %s" % (item["job_id"], first_line))
        lines.append("  output files        : %d" % len(self.outputs))
        lines.append("  total output size   : %s"
                     % _human_size(self.total_output_bytes))
        lines.append("  elapsed             : %.1f s" % self.elapsed_s)
        return "\n".join(lines)


def _human_size(size):
    if size < 1024:
        return "%d B" % size
    value = float(size)
    for unit in ("KiB", "MiB", "GiB"):
        value /= 1024.0
        if value < 1024.0 or unit == "GiB":
            return "%.1f %s" % (value, unit)
