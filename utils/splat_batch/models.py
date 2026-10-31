"""Plain data containers shared between the splat-batch modules."""

from dataclasses import dataclass, field
from typing import Optional


@dataclass
class Repeater:
    """One repeater record as read from an input file.

    A record carries exactly one output frequency; inputs that list several
    frequencies are expanded into several records by the parsers.
    """

    callsign: str
    lat: Optional[float]
    lon: Optional[float]
    frequency_mhz: Optional[float]
    modulation: str = ""
    flags: str = ""
    county: str = ""
    source: str = ""
    elevation_m: Optional[float] = None
    height_m: Optional[float] = None
    resolved_lat: Optional[float] = None
    resolved_lon: Optional[float] = None

    @property
    def has_resolved_position(self):
        return self.resolved_lat is not None and self.resolved_lon is not None


@dataclass
class SkipRecord:
    """A repeater (or group of records) that was not turned into a job."""

    callsign: str
    frequency_mhz: Optional[float]
    county: str
    reason: str
    detail: str = ""

    def as_dict(self):
        return {
            "callsign": self.callsign,
            "frequency_mhz": self.frequency_mhz,
            "county": self.county,
            "reason": self.reason,
            "detail": self.detail,
        }


@dataclass
class Job:
    """A single SPLAT! run: one callsign on one output frequency."""

    job_id: str
    callsign: str
    county: str
    lat: float
    lon: float
    frequency_mhz: float
    band: str
    hear_db: float
    talk_db: float
    tx_height_m: float
    rx_height_m: float
    radius_km: float
    climate: int
    maxpages: int
    workdir: str = ""
    features_file: str = ""
    cache_key: str = ""
    extra: dict = field(default_factory=dict)


@dataclass
class JobResult:
    """Outcome of executing one job in a worker process."""

    job_id: str
    status: str  # "ok" or "failed"
    features_file: str = ""
    feature_count: int = 0
    error: str = ""
    elapsed_s: float = 0.0
    log_file: str = ""
    itm_error: int = 0  # highest ITM/ITWOM error number reported by SPLAT!
