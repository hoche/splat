"""Start-up dependency checks with clear error messages."""

import importlib
import os
import shutil

from .postprocess import find_polygonize


def check_dependencies(config):
    """Return a list of human-readable problems (empty when all is well)."""
    problems = []

    for module, package in (("numpy", "numpy"), ("shapely", "shapely")):
        try:
            importlib.import_module(module)
        except ImportError:
            problems.append("Python module '%s' is not installed "
                            "(pip install %s)" % (module, package))

    tools = config["tools"]
    if shutil.which(tools["gdal_translate"]) is None:
        problems.append("GDAL command '%s' not found in PATH "
                        "(install the GDAL command-line utilities)"
                        % tools["gdal_translate"])
    if find_polygonize(config) is None:
        problems.append("GDAL command gdal_polygonize.py (or gdal_polygonize) "
                        "not found in PATH (install gdal-bin / python3-gdal)")
    if ("flatgeobuf" in config["output"]["formats"] and
            shutil.which(tools["ogr2ogr"]) is None):
        problems.append("GDAL command '%s' not found in PATH; it is required "
                        "for FlatGeobuf output" % tools["ogr2ogr"])

    splat_bin = config["splat_bin"]
    if shutil.which(splat_bin) is None and not os.path.isfile(splat_bin):
        problems.append("SPLAT! binary '%s' not found (use --splat-bin)"
                        % splat_bin)

    mt = config["mapterhorn"]
    if mt["enabled"] and shutil.which(mt["bin"]) is None:
        problems.append("terrain tool '%s' not found (build mapterhorn2sdf "
                        "or set mapterhorn.bin)" % mt["bin"])
    return problems
