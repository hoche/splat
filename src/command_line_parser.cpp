/** @file command_line_parser.cpp
 *
 * File created by Michel Hoche-Mong (KF6ZST) 12/16/2025
 * Derived from original project code.
 * Splat!
 * @copyright 1997 - 2018 John A. Magliacane (KD2BD) and contributors.
 * See revision control history for contributions.
 * This file is covered by the LICENSE.md file in the root of this project.
 */

#include "command_line_parser.h"
#include "splat_run.h"
#include <CLI/CLI.hpp>
#include <iostream>

void SetupCLI11(CLI::App &app, SplatRun &sr, CommandLineOptions &options) {
    app.description("SPLAT!: An RF Signal Path Loss And Terrain Analysis Tool");
    app.set_version_flag("--version", std::string(SplatRun::splat_name) + " v" +
                                          std::string(SplatRun::splat_version));

    // Disable default help flags and add custom one
    app.set_help_flag();  // Clear default -h,--help
    app.set_help_all_flag("--help,-?", "Print help message and exit");

    // Required options
    app.add_option("-t,--transmitter", options.tx_site_files,
                   "Transmitter site(s) .qth file(s)")
        ->required();

    // Site and location options
    app.add_option("-r,--receiver", options.rx_site_file,
                   "Receiver site .qth file");

    app.add_option("-s,--cities", options.city_files,
                   "City/site file(s) to import (max 5)");

    app.add_option("-b,--boundaries", options.boundary_files,
                   "Cartographic boundary file(s) to import (max 5)");

    // Coverage and analysis modes
    auto coverage_opt = app.add_option("-c,--coverage", sr.altitude,
                                       "Plot LOS coverage with RX antenna at "
                                       "specified height (feet/meters AGL)");

    auto pathloss_opt = app.add_option(
        "-L,--pathloss", sr.altitudeLR,
        "Plot path loss map with RX at specified height (feet/meters AGL)");

    // Note: Original SPLAT! allows both -c and -L, with a warning
    // CLI11 would make them mutually exclusive, but for backward compatibility:
    // coverage_opt->excludes(pathloss_opt);
    // pathloss_opt->excludes(coverage_opt);

    // Plot output options
    app.add_option("-p,--terrain-profile", options.terrain_file,
                   "Filename of terrain profile graph to plot");

    app.add_option("-e,--elevation-plot", options.elevation_file,
                   "Filename of terrain elevation graph to plot");

    // Note: Can't use -h (reserved for help), use long form only
    auto height_opt =
        app.add_option("--height-plot", options.height_file,
                       "Filename of terrain height graph to plot");

    auto norm_height_opt =
        app.add_option("-H,--normalized-height", options.height_file,
                       "Filename of normalized terrain height graph to plot");

    height_opt->excludes(norm_height_opt);
    norm_height_opt->excludes(height_opt);

    app.add_option("-l,--longley-plot", options.longley_file,
                   "Filename of path loss graph to plot");

    app.add_option("-o,--output-map", options.mapfile,
                   "Filename of topographic map to generate (without suffix)");

    // Data file options
    app.add_option("-d,--sdf-path", sr.sdf_path,
                   "SDF file directory path (overrides ~/.splat_path)");

    app.add_option("--udt,--user-terrain", options.udt_file,
                   "User-defined terrain input file");

    app.add_option("--ani,--alphanum-input", options.ani_filename,
                   "Alphanumeric input file from previous simulation");

    app.add_option("--ano,--alphanum-output", options.ano_filename,
                   "Alphanumeric output file");

    // Propagation parameters - with clamping to match original behavior
    app.add_option("-f,--frequency", sr.forced_freq,
                   "Frequency for Fresnel zone calculation (MHz)");

    app.add_option("-R,--range", sr.max_range,
                   "Maximum analysis range (miles/kilometers)");

    app.add_option("-m,--earth-multiplier", sr.er_mult,
                   "Earth radius multiplier (for atmospheric refraction)")
        ->default_val(1.0);

    app.add_option("--erp,--effective-power", sr.forced_erp,
                   "Override ERP in .lrp file (Watts)");

    app.add_option("--fz,--fresnel-clearance", sr.fzone_clearance,
                   "Fresnel zone clearance percentage (default 60)");

    app.add_option("--gc,--ground-clutter", sr.clutter,
                   "Ground clutter height (feet/meters)");

    app.add_option("--db,--contour-threshold", sr.contour_threshold,
                   "Threshold beyond which contours will not be displayed");

    // Display and output flags
    app.add_flag("-n,--no-los-path", sr.nolospath,
                 "Do not plot LOS paths in maps");

    app.add_flag(
        "-N,--no-reports",
        [&sr](size_t) {
            sr.nolospath = true;
            sr.nositereports = true;
        },
        "Do not produce site or obstruction reports");

    app.add_flag(
        "--nf,--no-fresnel", [&sr](size_t) { sr.fresnel_plot = false; },
        "Do not plot Fresnel zones in height plots");

    app.add_flag("--sc,--smooth-contours", sr.smooth_contours,
                 "Display smooth rather than quantized contour levels");

    app.add_flag("--ngs,--no-greyscale", sr.ngs,
                 "Display greyscale topography as white in images");

    // Image format options
    auto ppm_flag = app.add_flag(
        "--ppm", [&sr](size_t) { sr.imagetype = IMAGETYPE_PPM; },
        "Generate PPM images");

    auto jpg_flag = app.add_flag(
        "--jpg", [&sr](size_t) { sr.imagetype = IMAGETYPE_JPG; },
        "Generate JPEG images");

    auto tif_flag = app.add_flag(
        "--tif", [&sr](size_t) { sr.imagetype = IMAGETYPE_GEOTIFF; },
        "Generate GeoTIFF images");

    // Make image formats mutually exclusive
    ppm_flag->excludes(jpg_flag)->excludes(tif_flag);
    jpg_flag->excludes(ppm_flag)->excludes(tif_flag);
    tif_flag->excludes(ppm_flag)->excludes(jpg_flag);

    app.add_option(
           "--proj,--projection",
           [&sr](CLI::results_t res) {
               std::string proj = res[0];
               if (proj == "epsg:3857") {
                   sr.projection = PROJ_EPSG_3857;
                   return true;
               } else if (proj == "epsg:4326") {
                   sr.projection = PROJ_EPSG_4326;
                   return true;
               }
               return false;
           },
           "Map projection (epsg:3857 or epsg:4326)")
        ->check(CLI::IsMember({"epsg:3857", "epsg:4326"}));

    // Output format options
    app.add_flag("--kml", sr.kml,
                 "Generate Google Earth (.kml) compatible output");

    app.add_flag("--kmz", sr.kmz,
                 "Generate Google Earth compressed (.kmz) output");

    app.add_flag("--geo", sr.geo, "Generate Xastir .geo georeference file");

    app.add_flag("--json", sr.json,
                 "Create JSON file containing configuration");

    app.add_flag(
        "--dbm", sr.dbm,
        "Plot signal power level contours (dBm) rather than field strength");

    app.add_option("--log,--command-log", options.logfile,
                   "Copy command line to this output file");

    // Execution options
    app.add_option("-v,--verbose", sr.verbose,
                   "Verbosity level (0=quiet, 1=normal)")
        ->default_val(1);

    app.add_flag(
        "--st,--single-thread", [&sr](size_t) { sr.multithread = false; },
        "Use single CPU thread (classic mode)");

    app.add_flag(
        "--hd,--high-definition", sr.hd_mode,
        "Use high definition mode (3600 ppd vs 1200 ppd, requires SRTM-1)");

    app.add_flag(
        "--itwom", [&sr](size_t) { sr.propagation_model = PROP_ITWOM; },
        "Use ITWOM propagation model instead of Longley-Rice");

    app.add_flag(
        "--imperial", [&sr](size_t) { sr.metric = false; },
        "Use imperial units rather than metric for all I/O");

    app.add_flag("--msl", sr.msl, "Use MSL for TX/RX altitudes instead of AGL");

    app.add_flag("--gpsav", sr.gpsav,
                 "Preserve gnuplot temporary working files after execution");

    app.add_option(
           "--maxpages", sr.maxpages,
           "Maximum analysis region capability (1, 4, 9, 16, 25, 36, 49, 64)")
        ->default_val(16);

    app.add_option("--sdelim,--sdf-delimiter", sr.sdf_delimiter,
                   "Latitude and longitude delimiter in SDF filenames")
        ->default_val("_");

    // Callback to set flags and clamp values based on parsed options
    app.parse_complete_callback([&sr, &options, &app, height_opt,
                                 norm_height_opt, coverage_opt,
                                 pathloss_opt]() {
        // Clamp frequency to valid range (20-20000 MHz)
        if (sr.forced_freq < 20.0)
            sr.forced_freq = 0.0;
        if (sr.forced_freq > 20.0e3)
            sr.forced_freq = 20.0e3;

        // Clamp range to valid range (0-1000)
        if (sr.max_range < 0.0)
            sr.max_range = 0.0;
        if (sr.max_range > 1000.0)
            sr.max_range = 1000.0;

        // Clamp earth radius multiplier
        if (sr.er_mult < 0.1)
            sr.er_mult = 1.0;
        if (sr.er_mult > 1.0e6)
            sr.er_mult = 1.0e6;

        // Clamp ERP
        if (sr.forced_erp < 0.0)
            sr.forced_erp = -1.0;

        // Clamp and convert Fresnel zone clearance
        if (app["--fz"]->count() > 0) {
            if (sr.fzone_clearance < 0.0 || sr.fzone_clearance > 100.0)
                sr.fzone_clearance = 0.6;  // Default 60% as decimal
            else
                sr.fzone_clearance /= 100.0;  // Convert percentage to decimal
        }

        // Clamp ground clutter
        if (sr.clutter < 0.0)
            sr.clutter = 0.0;

        // Set mode flags based on what was parsed
        if (coverage_opt->count() > 0) {
            sr.map = true;
            sr.coverage = true;
            sr.area_mode = true;
        }

        if (pathloss_opt->count() > 0) {
            sr.map = true;
            sr.LRmap = true;
            sr.area_mode = true;
        }

        if (! options.terrain_file.empty()) {
            sr.terrain_plot = true;
            sr.pt2pt_mode = true;
        }

        if (! options.elevation_file.empty()) {
            sr.elevation_plot = true;
            sr.pt2pt_mode = true;
        }

        if (! options.height_file.empty()) {
            sr.height_plot = true;
            sr.pt2pt_mode = true;
            sr.norm = (norm_height_opt->count() > 0);
        }

        if (! options.longley_file.empty()) {
            sr.longley_plot = true;
            sr.pt2pt_mode = true;
        }

        if (! options.rx_site_file.empty()) {
            sr.rxsite = true;
            sr.pt2pt_mode = true;
        }

        if (! options.mapfile.empty()) {
            sr.map = true;
        }

        if (! options.logfile.empty()) {
            sr.command_line_log = true;
        }

        // Apply earth radius multiplier
        if (app["--earth-multiplier"]->count() > 0) {
            sr.earthradius *= sr.er_mult;
        }
    });
}

bool ParseCommandLine(int argc, const char *argv[], SplatRun &sr,
                      CommandLineOptions &options) {
    CLI::App app;
    SetupCLI11(app, sr, options);

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError &e) {
        // Handle help and version specially
        if (e.get_exit_code() == static_cast<int>(CLI::ExitCodes::Success)) {
            options.show_help = true;
            return false;
        }

        // Handle errors
        options.parse_error = true;
        options.error_message = e.what();
        std::cerr << app.help() << std::endl;
        return false;
    }

    return true;
}

bool ValidateCommandLine(const SplatRun &sr,
                         const CommandLineOptions &options) {
    // Check for transmitter sites
    if (options.tx_site_files.empty()) {
        std::cerr << "\n*** ERROR: No transmitter site(s) specified!\n\n";
        return false;
    }

    // Validate maxpages is one of the allowed values
    switch (sr.maxpages) {
    case 1:
    case 4:
    case 9:
    case 16:
    case 25:
    case 36:
    case 49:
    case 64:
        break;
    default:
        std::cerr
            << "\n*** ERROR: -maxpages must be one of 1, 4, 9, 16, 25, 36, "
               "49, 64\n\n";
        return false;
    }

    // Validate maxpages with HD mode
    if (sr.maxpages == 1 && ! sr.hd_mode) {
        std::cerr
            << "\n*** ERROR: -maxpages must be >= 4 if not in HD mode!\n\n";
        return false;
    }

    return true;
}
