"""splat-batch: batch coverage generation driver for SPLAT!.

One independent SPLAT! run is performed per repeater (callsign plus output
frequency).  Results are post-processed into hear/talk polygons and written
per county as GeoJSON and FlatGeobuf.
"""

__version__ = "0.1.0"
