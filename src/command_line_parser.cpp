/** @file command_line_parser.cpp
 *
 * Command-line argument parser for SPLAT!
 * 
 * @copyright 1997 - 2018 John A. Magliacane (KD2BD) and contributors.
 * See revision control history for contributions.
 * This file is covered by the LICENSE.md file in the root of this project.
 */

#include "command_line_parser.h"
#include "splat_run.h"
#include <cstring>
#include <iostream>

void PrintHelp(const SplatRun &sr) {
    std::cout
        << "\n\t\t --==[ " << SplatRun::splat_name << " v"
        << SplatRun::splat_version
        << " Available Options... ]==--\n\n"
           "       -t txsite(s).qth\n"
           "       -r sr.rxsite.qth\n"
           "       -c plot LOS coverage of TX(s) with an RX antenna at X "
           "feet/meters AGL\n"
           "       -L plot path loss map of TX based on an RX at X "
           "feet/meters AGL\n"
           "       -s filename(s) of city/site file(s) to import (5 max)\n"
           "       -b filename(s) of cartographic boundary file(s) to "
           "import (5 max)\n"
           "       -p filename of terrain profile graph to plot\n"
           "       -e filename of terrain elevation graph to plot\n"
           "       -h filename of terrain height graph to plot\n"
           "       -H filename of normalized terrain height graph to plot\n"
           "       -l filename of path loss graph to plot\n"
           "       -o filename of topographic map to generate (without "
           "suffix)\n"
           "       -d sdf file directory path (overrides path in "
           "~/.splat_path file)\n"
           "       -m earth radius multiplier\n"
           "       -n do not plot LOS paths in maps\n"
           "       -N do not produce unnecessary site or obstruction "
           "reports\n"
           "       -f frequency for Fresnel zone calculation (MHz)\n"
           "       -R modify default range for -c or -L "
           "(miles/kilometers)\n"
           "       -v N verbosity level. Default is 1. Set to 0 to quiet "
           "everything.\n"
           "      -st use a single CPU thread (classic mode)\n"
           "      -hd Use High Definition mode (3600 ppd vs 1200 ppd). "
           "Requires SRTM-1 SDF files.\n"
           "      -sc display smooth rather than quantized contour levels\n"
           "      -db threshold beyond which contours will not be "
           "displayed\n"
           "      -nf do not plot Fresnel zones in height plots\n"
           "      -fz Fresnel zone clearance percentage (default = 60)\n"
           "      -gc ground clutter height (feet/meters)\n"
           "     -jpg when generating maps, create jpgs instead of pngs or "
           "ppms\n"
#ifdef HAVE_LIBPNG
           "     -ppm when generating maps, create ppms instead of pngs or "
           "jpgs\n"
#endif
           "     -tif create geotiff instead of png or jpeg\n"
           "     -ngs display greyscale topography as white in images\n"
           "     -erp override ERP in .lrp file (Watts)\n"
           "     -ano name of alphanumeric output file\n"
           "     -ani name of alphanumeric input file\n"
           "     -udt name of user defined terrain input file\n"
           "     -kml generate Google Earth (.kml) compatible output\n"
           "     -kmz generate Google Earth compressed (.kmz) output\n"
           "     -geo generate an Xastir .geo georeference file (with "
           "image output)\n"
           "     -dbm plot signal power level contours rather than field "
           "strength\n"
           "     -log copy command line std::string to this output file\n"
           "     -json create JSON file containing configuration \n"
           "   -gpsav preserve gnuplot temporary working files after "
           "SPLAT! execution\n"
           "   -itwom invoke the ITWOM model instead of using "
           "Longley-Rice\n"
           "  -imperial employ imperial rather than metric units for all "
           "user I/O\n"
           "  -msl use MSL for TX/RX altitudes instead of AGL\n"
           "-maxpages ["
        << sr.maxpages
        << "] Maximum Analysis Region capability: 1, 4, 9, 16, 25, 36, 49, "
           "64 \n"
           "  -sdelim ["
        << sr.sdf_delimiter
        << "] Lat and lon delimeter in SDF filenames \n"
           "\n"
           "See the documentation for more details.\n\n";
}

bool ParseCommandLine(int argc, const char *argv[], SplatRun &sr,
                      CommandLineOptions &options) {
    // Check for help or no arguments
    if (argc == 1 || (argc == 2 && strcmp(argv[1], "--help") == 0)) {
        options.show_help = true;
        PrintHelp(sr);
        return false;
    }

    size_t x, y, z = 0;

    /* Scan for command line arguments */
    y = argc - 1;

    for (x = 1; x <= y; x++) {
        if (strcmp(argv[x], "-R") == 0) {
            z = x + 1;

            if (z <= y && argv[z][0] && argv[z][0] != '-') {
                sscanf(argv[z], "%lf", &sr.max_range);

                if (sr.max_range < 0.0)
                    sr.max_range = 0.0;

                if (sr.max_range > 1000.0)
                    sr.max_range = 1000.0;
            }
        }

        if (strcmp(argv[x], "-m") == 0) {
            z = x + 1;

            if (z <= y && argv[z][0] && argv[z][0] != '-') {
                sscanf(argv[z], "%lf", &sr.er_mult);

                if (sr.er_mult < 0.1)
                    sr.er_mult = 1.0;

                if (sr.er_mult > 1.0e6)
                    sr.er_mult = 1.0e6;

                sr.earthradius *= sr.er_mult;
            }
        }

        if (strcmp(argv[x], "-v") == 0) {
            z = x + 1;

            if (z < (size_t)argc && argv[z][0] && argv[z][0] != '-') {
                int verbose;
                sscanf(argv[z], "%d", &verbose);
                sr.verbose = verbose != 0;
            }
        }

        if (strcmp(argv[x], "-gc") == 0) {
            z = x + 1;

            if (z <= y && argv[z][0] && argv[z][0] != '-') {
                sscanf(argv[z], "%lf", &sr.clutter);

                if (sr.clutter < 0.0)
                    sr.clutter = 0.0;
            }
        }

        if (strcmp(argv[x], "-fz") == 0) {
            z = x + 1;

            if (z <= y && argv[z][0] && argv[z][0] != '-') {
                sscanf(argv[z], "%lf", &sr.fzone_clearance);

                if (sr.fzone_clearance < 0.0 || sr.fzone_clearance > 100.0)
                    sr.fzone_clearance = 60.0;

                sr.fzone_clearance /= 100.0;
            }
        }

        if (strcmp(argv[x], "-o") == 0) {
            z = x + 1;

            if (z <= y && argv[z][0] && argv[z][0] != '-')
                options.mapfile = argv[z];
            sr.map = true;
        }

        if (strcmp(argv[x], "-log") == 0) {
            z = x + 1;

            if (z <= y && argv[z][0] && argv[z][0] != '-')
                options.logfile = argv[z];

            sr.command_line_log = true;
        }

        if (strcmp(argv[x], "-udt") == 0) {
            z = x + 1;

            if (z <= y && argv[z][0] && argv[z][0] != '-')
                options.udt_file = argv[z];
        }

        if (strcmp(argv[x], "-c") == 0) {
            z = x + 1;

            if (z <= y && argv[z][0] && argv[z][0] != '-') {
                sscanf(argv[z], "%lf", &sr.altitude);
                sr.map = true;
                sr.coverage = true;
                sr.area_mode = true;
            }
        }

        if (strcmp(argv[x], "-db") == 0 || strcmp(argv[x], "-dB") == 0) {
            z = x + 1;

            if (z <= y && argv[z][0]) /* A minus argument is legal here */
                sscanf(argv[z], "%d", &sr.contour_threshold);
        }

        if (strcmp(argv[x], "-p") == 0) {
            z = x + 1;

            if (z <= y && argv[z][0] && argv[z][0] != '-') {
                options.terrain_file = argv[z];
                sr.terrain_plot = true;
                sr.pt2pt_mode = true;
            }
        }

        if (strcmp(argv[x], "-e") == 0) {
            z = x + 1;

            if (z <= y && argv[z][0] && argv[z][0] != '-') {
                options.elevation_file = argv[z];
                sr.elevation_plot = true;
                sr.pt2pt_mode = true;
            }
        }

        if (strcmp(argv[x], "-h") == 0 || strcmp(argv[x], "-H") == 0) {
            z = x + 1;

            if (z <= y && argv[z][0] && argv[z][0] != '-') {
                options.height_file = argv[z];
                sr.height_plot = true;
                sr.pt2pt_mode = true;
            }

            sr.norm = strcmp(argv[x], "-H") == 0 ? true : false;
        }

        bool imagetype_set = false;
#ifdef HAVE_LIBPNG
        if (strcmp(argv[x], "-ppm") == 0) {
            if (imagetype_set && sr.imagetype != IMAGETYPE_PPM) {
                fprintf(stdout,
                        "-jpg and -ppm are exclusive options, ignoring -ppm.\n");
            } else {
                sr.imagetype = IMAGETYPE_PPM;
                imagetype_set = true;
            }
        }
#endif
#ifdef HAVE_LIBGDAL
        if (strcmp(argv[x], "-tif") == 0) {
            if (imagetype_set && sr.imagetype != IMAGETYPE_PPM) {
                fprintf(stdout,
                        "-tif and -ppm are exclusive options, ignoring -ppm.\n");
            } else {
                sr.imagetype = IMAGETYPE_GEOTIFF;
                imagetype_set = true;
            }
        }
#endif
#ifdef HAVE_LIBJPEG
        if (strcmp(argv[x], "-jpg") == 0) {
            if (imagetype_set && sr.imagetype != IMAGETYPE_JPG) {
#ifdef HAVE_LIBPNG
                fprintf(stdout,
                        "-jpg and -ppm are exclusive options, ignoring -jpg.\n");
#else
                fprintf(stdout,
                        "-jpg and -png are exclusive options, ignoring -jpg.\n");
#endif
            } else {
                sr.imagetype = IMAGETYPE_JPG;
                imagetype_set = true;
            }
        }
#endif

#ifdef HAVE_LIBGDAL
        if (strcmp(argv[x], "-proj") == 0) {
            if (sr.imagetype == IMAGETYPE_GEOTIFF ||
                sr.imagetype == IMAGETYPE_PNG || sr.imagetype == IMAGETYPE_JPG) {
                z = x + 1;
                if (z <= y && argv[z][0] && argv[z][0] != '-') {
                    if (strcmp(argv[z], "epsg:3857") == 0) {
                        sr.projection = PROJ_EPSG_3857;
                    } else if (strcmp(argv[z], "epsg:4326") == 0) {
                        sr.projection = PROJ_EPSG_4326;
                    } else {
                        std::cerr << "Ignoring unknown projection " << argv[z]
                                  << " and taking epsg:4326 instead.\n";
                    }
                }
            } else {
                std::cerr << "-proj supports only gdal output formats. Please "
                             "use -png, -tif or -jpg.\n";
            }
        }
#endif

        if (strcmp(argv[x], "-imperial") == 0)
            sr.metric = false;

        if (strcmp(argv[x], "-msl") == 0)
            sr.msl = true;

        if (strcmp(argv[x], "-gpsav") == 0)
            sr.gpsav = true;

        if (strcmp(argv[x], "-geo") == 0)
            sr.geo = true;

        if (strcmp(argv[x], "-kml") == 0)
            sr.kml = true;

        if (strcmp(argv[x], "-kmz") == 0)
            sr.kmz = true;

        if (strcmp(argv[x], "-json") == 0)
            sr.json = true;

        if (strcmp(argv[x], "-nf") == 0)
            sr.fresnel_plot = false;

        if (strcmp(argv[x], "-ngs") == 0)
            sr.ngs = true;

        if (strcmp(argv[x], "-n") == 0)
            sr.nolospath = true;

        if (strcmp(argv[x], "-dbm") == 0)
            sr.dbm = true;

        if (strcmp(argv[x], "-sc") == 0)
            sr.smooth_contours = true;

        if (strcmp(argv[x], "-st") == 0)
            sr.multithread = false;

        if (strcmp(argv[x], "-itwom") == 0)
            sr.propagation_model = PROP_ITWOM;

        if (strcmp(argv[x], "-N") == 0) {
            sr.nolospath = true;
            sr.nositereports = true;
        }

        if (strcmp(argv[x], "-d") == 0) {
            z = x + 1;

            if (z <= y && argv[z][0] && argv[z][0] != '-')
                sr.sdf_path = argv[z];
        }

        if (strcmp(argv[x], "-t") == 0) {
            /* Read Transmitter Location */

            z = x + 1;

            while (z <= y && argv[z][0] && argv[z][0] != '-' &&
                   options.tx_site_files.size() < 30) {
                options.tx_site_files.push_back(argv[z]);
                z++;
            }

            z--;
        }

        if (strcmp(argv[x], "-L") == 0) {
            z = x + 1;

            if (z <= y && argv[z][0] && argv[z][0] != '-') {
                sscanf(argv[z], "%lf", &sr.altitudeLR);
                sr.map = true;
                sr.LRmap = true;
                sr.area_mode = true;

                if (sr.coverage)
                    fprintf(stdout,
                            "c and L are exclusive options, ignoring L.\n");
            }
        }

        if (strcmp(argv[x], "-l") == 0) {
            z = x + 1;

            if (z <= y && argv[z][0] && argv[z][0] != '-') {
                options.longley_file = argv[z];
                sr.longley_plot = true;
                sr.pt2pt_mode = true;
            }
        }

        if (strcmp(argv[x], "-r") == 0) {
            /* Read Receiver Location */

            z = x + 1;

            if (z <= y && argv[z][0] && argv[z][0] != '-') {
                options.rx_site_file = argv[z];
                sr.rxsite = true;
                sr.pt2pt_mode = true;
            }
        }

        if (strcmp(argv[x], "-s") == 0) {
            /* Read city file(s) */

            z = x + 1;

            while (z <= y && argv[z][0] && argv[z][0] != '-') {
                options.city_files.push_back(argv[z]);
                z++;
            }

            z--;
        }

        if (strcmp(argv[x], "-b") == 0) {
            /* Read Boundary File(s) */

            z = x + 1;

            while (z <= y && argv[z][0] && argv[z][0] != '-') {
                options.boundary_files.push_back(argv[z]);
                z++;
            }

            z--;
        }

        if (strcmp(argv[x], "-f") == 0) {
            z = x + 1;

            if (z <= y && argv[z][0] && argv[z][0] != '-') {
                sscanf(argv[z], "%lf", &(sr.forced_freq));

                if (sr.forced_freq < 20.0)
                    sr.forced_freq = 0.0;

                if (sr.forced_freq > 20.0e3)
                    sr.forced_freq = 20.0e3;
            }
        }

        if (strcmp(argv[x], "-erp") == 0) {
            z = x + 1;

            if (z <= y && argv[z][0] && argv[z][0] != '-') {
                sscanf(argv[z], "%lf", &(sr.forced_erp));

                if (sr.forced_erp < 0.0)
                    sr.forced_erp = -1.0;
            }
        }

        if (strcmp(argv[x], "-ano") == 0) {
            z = x + 1;

            if (z <= y && argv[z][0] && argv[z][0] != '-')
                options.ano_filename = argv[z];
        }

        if (strcmp(argv[x], "-ani") == 0) {
            z = x + 1;

            if (z <= y && argv[z][0] && argv[z][0] != '-')
                options.ani_filename = argv[z];
        }

        if (strcmp(argv[x], "-maxpages") == 0) {
            z = x + 1;

            if (z <= y && argv[z][0] && argv[z][0] != '-') {
                std::string maxpages_str = argv[z];
                if (sscanf(maxpages_str.c_str(), "%d", &sr.maxpages) != 1) {
                    options.parse_error = true;
                    options.error_message =
                        "Could not parse maxpages: " + maxpages_str;
                    return false;
                }
            }
        }

        if (strcmp(argv[x], "-sdelim") == 0) {
            z = x + 1;

            if (z <= y && argv[z][0] && argv[z][0] != '-') {
                sr.sdf_delimiter = argv[z];
            }
        }

        if (strcmp(argv[x], "-hd") == 0) {
            sr.hd_mode = true;
        }
    } /* end of command line argument scanning */

    return true;
}

bool ValidateCommandLine(const SplatRun &sr, const CommandLineOptions &options) {
    // Check for transmitter sites
    if (options.tx_site_files.empty()) {
        std::cerr << "\n*** ERROR: No transmitter site(s) specified!\n\n";
        return false;
    }

    // Validate maxpages
    switch (sr.maxpages) {
    case 1:
        if (!sr.hd_mode) {
            std::cerr
                << "\n*** ERROR: -maxpages must be >= 4 if not in HD mode!\n\n";
            return false;
        }
        break;
    case 4:
    case 9:
    case 16:
    case 25:
    case 36:
    case 49:
    case 64:
        break;
    default:
        std::cerr << "\n*** ERROR: -maxpages must be one of 1, 4, 9, 16, 25, 36, "
                     "49, 64\n\n";
        return false;
    }

    return true;
}
