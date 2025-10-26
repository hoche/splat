/** @file main.cpp
 *
 * SPLAT!: An RF Signal Path Loss And Terrain Analysis Tool
 * Project started in 1997 by John A. Magliacane (KD2BD)
 * File created by Peter Watkins (KE7IST) 6/17/15.
 * Derived from original project code.
 * Splat!
 * @copyright 1997 - 2018 John A. Magliacane (KD2BD) and contributors.
 * See revision control history for contributions.
 * This file is covered by the LICENSE.md file in the root of this project.
 */

#include "anf.h"
#include "antenna_pattern.h"
#include "boundary_file.h"
#include "city_file.h"
#include "dem.h"
#include "elevation_map.h"
#include "gnuplot.h"
#include "image.h"
#include "itwom3.0.h"
#include "json.h"
#include "kml.h"
#include "lrp.h"
#include "path.h"
#include "region.h"
#include "report.h"
#include "sdf.h"
#include "site.h"
#include "splat_run.h"
#include "udt.h"
#include "utilities.h"
#include <bzlib.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

void check_allocation(void *ptr, string name, const SplatRun &sr);

int main(int argc, const char *argv[]) {
    size_t x, y, z = 0;
    int min_lat, min_lon, max_lat, max_lon, rxlat, rxlon, txlat, txlon,
        west_min, west_max, north_min, north_max;

    char *env = NULL;
    string mapfile, elevation_file, height_file, longley_file, terrain_file,
        udt_file, ani_filename, ano_filename, logfile, maxpages_str, proj;

    vector<std::string> city_file;
    vector<std::string> boundary_file;

    vector<Site> tx_site;
    Site rx_site;

    SplatRun sr;

    sr.maxpages = 16;
    sr.arraysize = -1;

	sr.propagation_model = PROP_ITM;
    sr.hd_mode = false;
    sr.coverage = false;
    sr.LRmap = false;
    sr.terrain_plot = false;
    sr.elevation_plot = false;
    sr.height_plot = false;
    sr.map = false;
    sr.longley_plot = false;
    sr.norm = false;
    sr.topomap = false;
    sr.geo = false;
    sr.kml = false;
    sr.json = false;
    sr.pt2pt_mode = false;
    sr.area_mode = false;
    sr.ngs = false;
    sr.nolospath = false;
    sr.nositereports = false;
    sr.fresnel_plot = true;
    sr.command_line_log = false;
    sr.rxsite = false;
    sr.metric = true;
    sr.dbm = false;
    sr.bottom_legend = true;
    sr.smooth_contours = false;

    sr.altitude = 0.0;
    sr.altitudeLR = 0.0;
    sr.tx_range = 0.0;
    sr.rx_range = 0.0;
    sr.deg_range = 0.0;
    sr.deg_limit = 0.0;
    sr.max_range = 0.0;
    sr.clutter = 0.0;
    sr.forced_erp = -1.0;
    sr.forced_freq = 0.0;
    sr.fzone_clearance = 0.6;
    sr.contour_threshold = 0;
    sr.rx_site.lat = 91.0;
    sr.rx_site.lon = 361.0;
    sr.earthradius = EARTHRADIUS;
#ifdef HAVE_LIBPNG
    sr.imagetype = IMAGETYPE_PNG;
#else
    sr.imagetype = IMAGETYPE_PPM;
#endif
	sr.projection = PROJ_EPSG_4326;
    sr.multithread = true;
    sr.verbose = 1;    
    sr.sdf_delimiter = "_";
    
    if (argc == 1 || (argc == 2 && strcmp(argv[1], "--help") == 0)) {
        cout
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
               "       -u filename of user-defined terrain file to import\n"
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
               "      -hd Use High Definition mode. Requires 1-deg SDF files.\n"
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
               "     -geo generate an Xastir .geo georeference file (with "
               "image output)\n"
               "     -dbm plot signal power level contours rather than field "
               "strength\n"
               "     -log copy command line string to this output file\n"
               "     -json create JSON file containing configuration \n"
               "   -gpsav preserve gnuplot temporary working files after "
               "SPLAT! execution\n"
               "   -itwom invoke the ITWOM model instead of using "
               "Longley-Rice\n"
               "  -imperial employ imperial rather than metric units for all "
               "user I/O\n"
               "-maxpages ["
            << sr.maxpages
            << "] Maximum Analysis Region capability: 1, 4, 9, 16, 25, 36, 49, "
               "64 \n"
               "  -sdelim ["
            << sr.sdf_delimiter
            << "] Lat and lon delimeter in SDF filenames \n"
               "\n"
               "See the documentation for more details.\n\n";

        return 1;
    }


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
                mapfile = argv[z];
            sr.map = true;
        }

        if (strcmp(argv[x], "-log") == 0) {
            z = x + 1;

            if (z <= y && argv[z][0] && argv[z][0] != '-')
                logfile = argv[z];

            sr.command_line_log = true;
        }

        if (strcmp(argv[x], "-udt") == 0) {
            z = x + 1;

            if (z <= y && argv[z][0] && argv[z][0] != '-')
                udt_file = argv[z];
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
                terrain_file = argv[z];
                sr.terrain_plot = true;
                sr.pt2pt_mode = true;
            }
        }

        if (strcmp(argv[x], "-e") == 0) {
            z = x + 1;

            if (z <= y && argv[z][0] && argv[z][0] != '-') {
                elevation_file = argv[z];
                sr.elevation_plot = true;
                sr.pt2pt_mode = true;
            }
        }

        if (strcmp(argv[x], "-h") == 0 || strcmp(argv[x], "-H") == 0) {
            z = x + 1;

            if (z <= y && argv[z][0] && argv[z][0] != '-') {
                height_file = argv[z];
                sr.height_plot = true;
                sr.pt2pt_mode = true;
            }

            sr.norm = strcmp(argv[x], "-H") == 0 ? true : false;
        }

		bool imagetype_set = false;
#ifdef HAVE_LIBPNG
        if (strcmp(argv[x], "-ppm") == 0) {
            if (imagetype_set && sr.imagetype != IMAGETYPE_PPM) {
                fprintf(stdout, "-jpg and -ppm are exclusive options, ignoring -ppm.\n");
            } else {
                sr.imagetype = IMAGETYPE_PPM;
                imagetype_set = true;
            }
        }
#endif
#ifdef HAVE_LIBGDAL
        if (strcmp(argv[x], "-tif") == 0) {
            if (imagetype_set && sr.imagetype != IMAGETYPE_PPM) {
                fprintf(stdout, "-tif and -ppm are exclusive options, ignoring -ppm.\n");
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
                fprintf(stdout, "-jpg and -ppm are exclusive options, ignoring -jpg.\n");
#else
                fprintf(stdout, "-jpg and -png are exclusive options, ignoring -jpg.\n");
#endif
            } else {
                sr.imagetype = IMAGETYPE_JPG;
                imagetype_set = true;
            }
        }
#endif

#ifdef HAVE_LIBGDAL
        if (strcmp(argv[x], "-proj") == 0) {
            if (sr.imagetype == IMAGETYPE_GEOTIFF || sr.imagetype == IMAGETYPE_PNG || sr.imagetype == IMAGETYPE_JPG) {
				z = x + 1;
				if (z <= y && argv[z][0] && argv[z][0] != '-') {
					if(strcmp(argv[z], "epsg:3857") == 0) {
						sr.projection = PROJ_EPSG_3857;
					} else if(strcmp(argv[z], "epsg:4326") == 0) {
						sr.projection = PROJ_EPSG_4326;
					} else {
						cerr << "Ignoring unknown projection " << argv[z] << " and taking epsg:4326 instead.\n";
					}
				}	
            } else {
                cerr << "-proj supports only gdal output formats. Please use -png, -tif or -jpg.\n";
            }
        }
#endif

        if (strcmp(argv[x], "-imperial") == 0)
            sr.metric = false;

        if (strcmp(argv[x], "-gpsav") == 0)
            sr.gpsav = true;

        if (strcmp(argv[x], "-geo") == 0)
            sr.geo = true;

        if (strcmp(argv[x], "-kml") == 0)
            sr.kml = true;
            
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

            while (z <= y && argv[z][0] && argv[z][0] != '-') {
                string txfile = argv[z];
                tx_site.push_back(Site(txfile));
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
                    fprintf(stdout,"c and L are exclusive options, ignoring L.\n");
            }
        }

        if (strcmp(argv[x], "-l") == 0) {
            z = x + 1;

            if (z <= y && argv[z][0] && argv[z][0] != '-') {
                longley_file = argv[z];
                sr.longley_plot = true;
                sr.pt2pt_mode = true;
            }
        }

        if (strcmp(argv[x], "-r") == 0) {
            /* Read Receiver Location */

            z = x + 1;

            if (z <= y && argv[z][0] && argv[z][0] != '-') {
                string rxfile = argv[z];
                rx_site.LoadQTH(rxfile);
                sr.rxsite = true;
                sr.pt2pt_mode = true;
            }
        }

        if (strcmp(argv[x], "-s") == 0) {
            /* Read city file(s) */

            z = x + 1;

            while (z <= y && argv[z][0] && argv[z][0] != '-') {
                city_file.push_back(argv[z]);
                z++;
            }

            z--;
        }

        if (strcmp(argv[x], "-b") == 0) {
            /* Read Boundary File(s) */

            z = x + 1;

            while (z <= y && argv[z][0] && argv[z][0] != '-') {
                boundary_file.push_back(argv[z]);
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
                ano_filename = argv[z];
        }

        if (strcmp(argv[x], "-ani") == 0) {
            z = x + 1;

            if (z <= y && argv[z][0] && argv[z][0] != '-')
                ani_filename = argv[z];
        }

        if (strcmp(argv[x], "-maxpages") == 0) {
            z = x + 1;

            if (z <= y && argv[z][0] && argv[z][0] != '-') {
                maxpages_str = argv[z];
                if (sscanf(maxpages_str.c_str(), "%d", &sr.maxpages) != 1) {
                    cerr << "\n"
                         << 7 << "*** ERROR: Could not parse maxpages: "
                         << maxpages_str << "\n\n";
                    exit(-1);
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


    /* Perform some error checking on the arguments
     and switches parsed from the command-line.
     If an error is encountered, print a message
     and exit gracefully. */

    if (tx_site.size() == 0) {
        fprintf(stderr, "\n%c*** ERROR: No transmitter site(s) specified!\n\n", 7);
        exit(-1);
    }

    for (x = 0, y = 0; x < tx_site.size(); x++) {
        if (tx_site[x].lat == 91.0 && tx_site[x].lon == 361.0) {
            fprintf(stderr, "\n*** ERROR: Transmitter site #%lu not found!", x + 1);
            y++;
        }
    }

    if (y) {
        fprintf(stderr, "%c\n\n", 7);
        exit(-1);
    }

    if (!sr.coverage && !sr.LRmap && ani_filename.empty() &&
        rx_site.lat == 91.0 && rx_site.lon == 361.0) {
        if (sr.max_range != 0.0 && tx_site.size() != 0) {
            /* Plot topographic map of radius "sr.max_range" */
            sr.map = false;
            sr.topomap = true;
        } else {
            fprintf(stderr, "\n%c*** ERROR: No receiver site found or specified!\n\n", 7);
            exit(-1);
        }
    }
    
    /* check if the output map should have a bottom legend */
    // TODO: PVW: LOS maps don't use a legend. Does sr.coverage detect those correctly?
    if (sr.kml || sr.geo || (sr.imagetype == IMAGETYPE_GEOTIFF) || sr.coverage) {
		sr.bottom_legend = false;
	} else {
		sr.bottom_legend = true;
	}

    switch (sr.maxpages) {
    case 1:
        if (!sr.hd_mode) {
            fprintf(
                stderr,
                "\n%c*** ERROR: -maxpages must be >= 4 if not in HD mode!\n\n",
                7);
            exit(-1);
        }
        sr.arraysize = 5092;
        break;
    case 4:
        sr.arraysize = sr.hd_mode ? 14844 : 4950;
        break;

    case 9:
        sr.arraysize = sr.hd_mode ? 32600 : 10870;
        break;

    case 16:
        sr.arraysize = sr.hd_mode ? 57713 : 19240;
        break;

    case 25:
        sr.arraysize = sr.hd_mode ? 90072 : 30025;
        break;

    case 36:
        sr.arraysize = sr.hd_mode ? 129650 : 43217;
        break;

    case 49:
        sr.arraysize = sr.hd_mode ? 176437 : 58813;
        break;

    case 64:
        sr.arraysize = sr.hd_mode ? 230430 : 76810;
        break;
    default:
        fprintf(stderr,
                "\n%c*** ERROR: -maxpages must be one of 1, 4, 9, 16, 25, 36, "
                "49, 64\n\n",
                7);
        exit(-1);
    }

    sr.ippd = sr.hd_mode ? 3600 : 1200; /* pixels per degree (integer) */

    int degrees = (int)sqrt((int)sr.maxpages);

    cout << "This invocation of " << SplatRun::splat_name
         << " supports analysis over a region of " << degrees << " square \n" << ((degrees == 1) ? "degree" : "degrees")
		 << " of terrain, and computes signal levels using ITWOM Version " << ITWOMVersion() << ".\n\n";

    sr.ppd = (double)sr.ippd; /* pixels per degree (double)  */
    sr.dpp = 1.0 / sr.ppd;    /* degrees per pixel */
    sr.mpi = sr.ippd - 1;     /* maximum pixel index per degree */

    /* No major errors were detected.  Whew!  :-) */

    /* Adjust input parameters if -imperial option is not used */

    if (sr.metric) {
        sr.altitudeLR /= METERS_PER_FOOT; /* meters --> feet */
        sr.max_range /= KM_PER_MILE;      /* kilometers --> miles */
        sr.altitude /= METERS_PER_FOOT;   /* meters --> feet */
        sr.clutter /= METERS_PER_FOOT;    /* meters --> feet */
    }

    /* If no SDF path was specified on the command line (-d), check
     for a path specified in the $HOME/.splat_path file.  If the
     file is not found, then sr.sdf_path[] remains NULL, and the
     current working directory is assumed to contain the SDF
     files. */

    if (sr.sdf_path.empty()) {
        env = getenv("HOME");
        std::string config_path = env;
        config_path += "/.splat_path";
        fstream fs;
        fs.open(config_path.c_str(), fstream::in);

        if (fs) {
            getline(fs, sr.sdf_path);
            fs.close();
        }
    }

    /* Ensure a trailing '/' is present in sr.sdf_path */

    if (!sr.sdf_path.empty() && (*sr.sdf_path.rbegin() != '/')) {
        sr.sdf_path += '/';
    }
    Sdf sdf(sr.sdf_path, sr);

    // Now print the header:
    cout << "\n\t\t--==[ Welcome To " << SplatRun::splat_name << " v"
         << SplatRun::splat_version << " ]==--\n\n";

    elev_t *elev = new elev_t[sr.arraysize + 10];
    check_allocation(elev, "elev", sr);

    ElevationMap *em_p = new ElevationMap(sr);
    check_allocation(em_p, "em_p", sr);

    Lrp lrp(sr.forced_erp, sr.forced_freq);
    BoundaryFile bf(sr);
    CityFile cf;
    Region region;

    if (!ani_filename.empty()) {
	/* read alphanumeric output file from previous simulations if given */
	
        // TODO: Here's an instance where reading the LRParms may say to load
        // a PAT file but it's never used. Refactor this.
        AntennaPattern pat = AntennaPattern();

        // TODO: Why only the first TX site?
        bool loadPat;
        string patFilename;
        lrp.ReadLRParm(tx_site[0], 0, loadPat, patFilename); /* Get ERP status */
        if (loadPat) {
            pat.LoadAntennaPattern(patFilename);
        }
        Anf anf(lrp, sr);

        y = anf.LoadANO(ani_filename, sdf, *em_p);

        for (x = 0; x < tx_site.size(); x++)
            em_p->PlaceMarker(tx_site[x]);

        if (sr.rxsite)
            em_p->PlaceMarker(rx_site);

        if (boundary_file.size() > 0) {
            for (x = 0; x < boundary_file.size(); x++) {
                bf.LoadBoundaries(boundary_file[x], *em_p);
			}
            fprintf(stdout, "\n");
            fflush(stdout);
        }

        if (city_file.size() > 0) {
            for (x = 0; x < city_file.size(); x++) {
                cf.LoadCities(city_file[x], *em_p);
			}
            fprintf(stdout, "\n");
            fflush(stdout);
        }

        Image image(sr, mapfile, tx_site, *em_p);
        if (lrp.erp == 0.0) {
            image.WriteCoverageMap(MAPTYPE_PATHLOSS, sr.imagetype, region);
        } else {
            if (sr.dbm)
                image.WriteCoverageMap(MAPTYPE_DBM, sr.imagetype, region);
            else
                image.WriteCoverageMap(MAPTYPE_DBUVM, sr.imagetype, region);
        }

        exit(0);
    }
    
    /* proceed for normal simulation */

    x = 0;
    y = 0;

    min_lat = 90;
    max_lat = -90;

    min_lon = (int)floor(tx_site[0].lon);
    max_lon = (int)floor(tx_site[0].lon);

    for (y = 0, z = 0; z < tx_site.size(); z++) {
        txlat = (int)floor(tx_site[z].lat);
        txlon = (int)floor(tx_site[z].lon);

        if (txlat < min_lat)
            min_lat = txlat;

        if (txlat > max_lat)
            max_lat = txlat;

        if (Utilities::LonDiff(txlon, min_lon) < 0.0)
            min_lon = txlon;

        if (Utilities::LonDiff(txlon, max_lon) >= 0.0)
            max_lon = txlon;
    }

    if (sr.rxsite) {
        rxlat = (int)floor(rx_site.lat);
        rxlon = (int)floor(rx_site.lon);

        if (rxlat < min_lat)
            min_lat = rxlat;

        if (rxlat > max_lat)
            max_lat = rxlat;

        if (Utilities::LonDiff(rxlon, min_lon) < 0.0)
            min_lon = rxlon;

        if (Utilities::LonDiff(rxlon, max_lon) >= 0.0)
            max_lon = rxlon;
    }

    /* Load the required SDF files */
    em_p->LoadTopoData(max_lon, min_lon, max_lat, min_lat, sdf);

    if (sr.area_mode || sr.topomap) {
        for (z = 0; z < tx_site.size(); z++) {
            /* "Ball park" estimates used to load any additional
             SDF files required to conduct this analysis. */

            sr.tx_range = sqrt(1.5 * (tx_site[z].alt + em_p->GetElevation(tx_site[z])));

            if (sr.LRmap) {
                sr.rx_range = sqrt(1.5 * sr.altitudeLR);
            } else {
                sr.rx_range = sqrt(1.5 * sr.altitude);
			}

            /* sr.deg_range determines the maximum
             amount of topo data we read */

            sr.deg_range = (sr.tx_range + sr.rx_range) / 57.0;

            /* sr.max_range regulates the size of the
             analysis.  A small, non-zero amount can
             be used to shrink the size of the analysis
             and limit the amount of topo data read by
             SPLAT!  A large number will increase the
             width of the analysis and the size of
             the map. */

            if (sr.max_range == 0.0) {
                sr.max_range = sr.tx_range + sr.rx_range;
			}
            sr.deg_range = sr.max_range / 57.0;

            /* Prevent the demand for a really wide coverage
             from allocating more "pages" than are available
             in memory. */

            switch (sr.maxpages) {
            case 1:
                sr.deg_limit = 0.125;
                break;

            case 2:
                sr.deg_limit = 0.25;
                break;

            case 4:
                sr.deg_limit = 0.5;
                break;

            case 9:
                sr.deg_limit = 1.0;
                break;

            case 16:
                sr.deg_limit = 1.5; /* WAS 2.0 */
                break;

            case 25:
                sr.deg_limit = 2.0; /* WAS 3.0 */
                break;

            case 36:
                sr.deg_limit = 2.5; /* New! */
                break;

            case 49:
                sr.deg_limit = 3.0; /* New! */
                break;

            case 64:
                sr.deg_limit = 3.5; /* New! */
                break;
            }

            if (fabs(tx_site[z].lat) < 70.0) {
                sr.deg_range_lon = sr.deg_range / cos(DEG2RAD * tx_site[z].lat);
            } else {
                sr.deg_range_lon = sr.deg_range / cos(DEG2RAD * 70.0);
			}
			
            /* Correct for squares in degrees not being square in miles */
            if (sr.deg_range > sr.deg_limit)
                sr.deg_range = sr.deg_limit;

            if (sr.deg_range_lon > sr.deg_limit)
                sr.deg_range_lon = sr.deg_limit;

            north_min = (int)floor(tx_site[z].lat - sr.deg_range);
            north_max = (int)floor(tx_site[z].lat + sr.deg_range);

            west_min = (int)floor(tx_site[z].lon - sr.deg_range_lon);

            while (west_min < 0)
                west_min += 360;

            while (west_min >= 360)
                west_min -= 360;

            west_max = (int)floor(tx_site[z].lon + sr.deg_range_lon);

            while (west_max < 0)
                west_max += 360;

            while (west_max >= 360)
                west_max -= 360;

            if (north_min < min_lat)
                min_lat = north_min;

            if (north_max > max_lat)
                max_lat = north_max;

            if (Utilities::LonDiff(west_min, min_lon) < 0.0)
                min_lon = west_min;

            if (Utilities::LonDiff(west_max, max_lon) >= 0.0)
                max_lon = west_max;
        }

        /* Load any additional SDF files, if required */
        em_p->LoadTopoData(max_lon, min_lon, max_lat, min_lat, sdf);
    }

    if (!udt_file.empty()) {
        Udt udt(sr);
        udt.LoadUDT(udt_file, *em_p);
    }

    /***** Let the SPLATting begin! *****/

    Report report(*em_p, sr);

    if (sr.pt2pt_mode) {
        em_p->PlaceMarker(rx_site);

        string ext;
        if (sr.terrain_plot) {
            /* Extract extension (if present)
             from "terrain_file" */

            ext = Utilities::DivideExtension(terrain_file, "png");
        }

        if (sr.elevation_plot) {
            /* Extract extension (if present)
             from "elevation_file" */
            ext = Utilities::DivideExtension(elevation_file, "png");
        }

        if (sr.height_plot) {
            /* Extract extension (if present)
             from "height_file" */

            ext = Utilities::DivideExtension(height_file, "png");
        }

        if (sr.longley_plot) {
            /* Extract extension (if present)
             from "longley_file" */

            ext = Utilities::DivideExtension(longley_file, "png");
        }

        for (x = 0; x < tx_site.size() && x < 4; x++) {
            em_p->PlaceMarker(tx_site[x]);

            if (!sr.nolospath) {
                switch (x) {
                case 0:
                    em_p->PlotPath(tx_site[x], rx_site, 1);
                    break;

                case 1:
                    em_p->PlotPath(tx_site[x], rx_site, 8);
                    break;

                case 2:
                    em_p->PlotPath(tx_site[x], rx_site, 16);
                    break;

                case 3:
                    em_p->PlotPath(tx_site[x], rx_site, 32);
                }
            }

            if (!sr.nositereports)
                report.SiteReport(tx_site[x]);

            if (sr.kml) {
                Kml kml(*em_p, sr);
                kml.WriteKML(tx_site[x], rx_site);
            }

            // If there's more than one TX site, put a dash-tx_site_number on
            // the end before the extension.
            std::string filename;
            std::ostringstream oss;
            if (tx_site.size() > 1)
                oss << "-" << x + 1;
            oss << "." << ext;

            AntennaPattern pat;
            if (!sr.nositereports) {
                filename = longley_file + oss.str();
                bool longly_file_exists = !longley_file.empty();

                bool loadPat;
                string patFilename;
                lrp.ReadLRParm(tx_site[x], longly_file_exists, loadPat,
                               patFilename);
                if (loadPat) {
                    pat.LoadAntennaPattern(patFilename);
                }
                report.PathReport(tx_site[x], rx_site, filename,
                                  longly_file_exists, elev, pat, lrp);
            } else {
                bool loadPat;
                string patFilename;
                lrp.ReadLRParm(tx_site[x], 1, loadPat, patFilename);
                if (loadPat) {
                    pat.LoadAntennaPattern(patFilename);
                }
                report.PathReport(tx_site[x], rx_site, filename, true, elev,
                                  pat, lrp);
            }

            GnuPlot gnuPlot(sr);

            if (sr.terrain_plot) {
                filename = terrain_file + oss.str();
                gnuPlot.GraphTerrain(tx_site[x], rx_site, filename, *em_p);
            }

            if (sr.elevation_plot) {
                filename = elevation_file + oss.str();
                gnuPlot.GraphElevation(tx_site[x], rx_site, filename, *em_p);
            }

            if (sr.height_plot) {
                filename = height_file + oss.str();
                gnuPlot.GraphHeight(tx_site[x], rx_site, filename,
                                    sr.fresnel_plot, sr.norm, *em_p, lrp);
            }
        }
    }

    if (sr.area_mode && !sr.topomap) {
        // Allocate the antenna pattern on the heap because it has a huge array
        // of floats that would otherwise be on the stack.
        AntennaPattern *p_pat = new AntennaPattern();
        for (x = 0; x < tx_site.size(); x++) {

            if (sr.coverage) {
                em_p->PlotLOSMap(tx_site[x], sr.altitude);
            } else {
                bool loadPat;
                string patFilename;
                char flag = lrp.ReadLRParm(tx_site[x], 1, loadPat, patFilename);
                if (loadPat) {
                    p_pat->LoadAntennaPattern(patFilename);
                }

                if (flag) {
                    em_p->PlotLRMap(tx_site[x], sr.altitudeLR, ano_filename, *p_pat, lrp);
                }
            }

            report.SiteReport(tx_site[x]);
        }
        delete p_pat;
    }

    if (sr.map || sr.topomap) {
        /* Label the map */

        if (!(sr.kml || sr.imagetype == IMAGETYPE_GEOTIFF)) {
            for (x = 0; x < tx_site.size(); x++)
                em_p->PlaceMarker(tx_site[x]);
        }

        if (city_file.size() > 0) {
            CityFile cityFile;

            for (y = 0; y < city_file.size(); y++)
                cityFile.LoadCities(city_file[y], *em_p);

            fprintf(stdout, "\n");
            fflush(stdout);
        }

        /* Load city and county boundary data files */

        if (boundary_file.size() > 0) {
            BoundaryFile boundaryFile(sr);

            for (y = 0; y < boundary_file.size(); y++)
                boundaryFile.LoadBoundaries(boundary_file[y], *em_p);

            fprintf(stdout, "\n");
            fflush(stdout);
        }

        /* Plot the map */
        Image image(sr, mapfile, tx_site, *em_p);
        if (sr.coverage || sr.pt2pt_mode || sr.topomap) {
            image.WriteCoverageMap(MAPTYPE_LOS, sr.imagetype, region);
            // TODO: PVW: Remove commented out line
            //image.WriteImage(sr.imagetype);
        } else {
            if (lrp.erp == 0.0)
                image.WriteCoverageMap(MAPTYPE_PATHLOSS, sr.imagetype, region);
            else if (sr.dbm)
                image.WriteCoverageMap(MAPTYPE_DBM, sr.imagetype, region);
            else
                image.WriteCoverageMap(MAPTYPE_DBUVM, sr.imagetype, region);
        }
    }    

    if (sr.command_line_log && !logfile.empty()) {
        fstream fs;
        fs.open(logfile.c_str(), fstream::out);

        // TODO: Should we fail silently if we can't open the logfile. Shouldn't
        // we WARN?
        if (fs) {
            for (x = 0; x < (size_t)argc; x++) {
                fs << argv[x] << " ";
			}
            fs << endl;
            fs.close();

            cout << "\nCommand-line parameter log written to: \"" << logfile << "\"\n";
        }
    }

    cout << endl;
    
    /* json input/output must be somewhere else, but this is how it goes: 
    //================
    arg_t args;
	
	string curr_arg = "";
	int curr_arg_i = 0;
	for(int i=0; i<argc; i++) {		// step through argv[] array
		string arg = argv[i];
		if(arg.find("-") == 0) {	// check if current argument has leading "-"
			curr_arg = arg.erase(0,1);	// remove leading "-" and save as new array entry
			curr_arg_i = i;		// save position for multiple parameters
			args[curr_arg] = "";
		} else {
			if (curr_arg != "") {
				if(i == (curr_arg_i + 1)) {
					args[curr_arg] = arg;	// if no "-" was found the current argument is considered as a value to the previous argument
				} else {
					args[curr_arg] += " " + arg;	// if no "-" was found the current argument is considered as a value to the previous argument
				}
			}
		}
	}
	// end argv[] reading

	// disabled for now - the parser hasn't been tested.
	// hoche 21 Jun 2023
    if (sr.json) {
        Json json(*em_p, sr);
        json.WriteJSON(args, tx_site[0], lrp, mapfile);
    }
    */

    //=====================

    /* That's all, folks! */

    delete em_p;

    // TODO: Why can't we clear. It complains about items already being
    // deleted?!
    // dem.clear();
    delete[] elev;

    return 0;
}

void check_allocation(void *ptr, string name, const SplatRun &sr) {
    if (ptr == NULL) {
        cerr << "\n\a*** ERROR: Could not allocate memory for " << name
             << " with -maxpages == " << sr.maxpages << "\n\n";
        exit(-1);
    }
}
