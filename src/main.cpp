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
#include "command_line_parser.h"
#include "dem.h"
#include "elevation_map.h"
#include "gnuplot.h"
#include "image.h"
#include "imagewriter.h"
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

void check_allocation(void *ptr, std::string name, const SplatRun &sr);

int main(int argc, const char *argv[]) {
    // Initialize GDAL library once at program startup
    // This must be done before any ImageWriter objects are created
    ImageWriter::InitializeGDAL();

    size_t x, y, z;
    int min_lat, min_lon, max_lat, max_lon, rxlat, rxlon, txlat, txlon,
        west_min, west_max, north_min, north_max;

    char *env = NULL;

    std::vector<Site> tx_site;
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
    sr.msl = false;
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

    // Parse command-line arguments
    CommandLineOptions options;
    if (!ParseCommandLine(argc, argv, sr, options)) {
        if (options.parse_error) {
            std::cerr << "\n*** ERROR: " << options.error_message << "\n\n";
            return -1;
        }
        return options.show_help ? 1 : -1;
    }

    // Load transmitter sites from parsed filenames
    for (const auto &txfile : options.tx_site_files) {
        tx_site.push_back(Site(txfile));
    }

    // Load receiver site if specified
    if (!options.rx_site_file.empty()) {
        rx_site.LoadQTH(options.rx_site_file);
    }

    // Validate the parsed configuration
    if (!ValidateCommandLine(sr, options)) {
        return -1;
    }

    /* Perform additional error checking on the loaded sites */
    for (x = 0, y = 0; x < tx_site.size(); x++) {
        if (tx_site[x].lat == 91.0 && tx_site[x].lon == 361.0) {
            fprintf(stderr, "\n*** ERROR: Transmitter site #%lu not found!",
                    x + 1);
            y++;
        }
    }

    if (y) {
        fprintf(stderr, "%c\n\n", 7);
        exit(-1);
    }

    if (!sr.coverage && !sr.LRmap && options.ani_filename.empty() &&
        rx_site.lat == 91.0 && rx_site.lon == 361.0) {
        if (sr.max_range != 0.0 && tx_site.size() != 0) {
            /* Plot topographic map of radius "sr.max_range" */
            sr.map = false;
            sr.topomap = true;
        } else {
            fprintf(stderr,
                    "\n%c*** ERROR: No receiver site found or specified!\n\n",
                    7);
            exit(-1);
        }
    }

    /* Adjust TX/RX for MSL */
    if (sr.msl) {
        for (x = 0; x < tx_site.size(); x++) {
            tx_site[x].amsl_flag = true;
        }
        rx_site.amsl_flag = true;
    }

    /* check if the output map should have a bottom legend */
    // TODO: PVW: LOS maps don't use a legend. Does sr.coverage detect those correctly?
    if (sr.kml || sr.geo || (sr.imagetype == IMAGETYPE_GEOTIFF) ||
        sr.coverage) {
        sr.bottom_legend = false;
    } else {
        sr.bottom_legend = true;
    }

    switch (sr.maxpages) {
    case 1:
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
    }

    sr.ippd = sr.hd_mode ? 3600 : 1200; /* pixels per degree (integer) */

    int degrees = (int) sqrt((int) sr.maxpages);

    std::cout << "This invocation of " << SplatRun::splat_name
              << " supports analysis over a region of " << degrees
              << " square \n"
              << ((degrees == 1) ? "degree" : "degrees")
              << " of terrain, and computes signal levels using ITWOM Version "
              << ITWOMVersion() << ".\n\n";

    sr.ppd = (double) sr.ippd; /* pixels per degree (double)  */
    sr.dpp = 1.0 / sr.ppd;     /* degrees per pixel */
    sr.mpi = sr.ippd - 1;      /* maximum pixel index per degree */

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
        std::fstream fs;
        fs.open(config_path.c_str(), std::fstream::in);

        if (fs) {
            getline(fs, sr.sdf_path);
            fs.close();
        }
    }

    /* Ensure a trailing '/' is present in sr.sdf_path */

    if (! sr.sdf_path.empty() && (*sr.sdf_path.rbegin() != '/')) {
        sr.sdf_path += '/';
    }
    Sdf sdf(sr.sdf_path, sr);

    // Now print the header:
    std::cout << "\n\t\t--==[ Welcome To " << SplatRun::splat_name << " v"
              << SplatRun::splat_version << " ]==--\n\n";

    elev_t *elev = new elev_t[sr.arraysize + 10];
    check_allocation(elev, "elev", sr);

    ElevationMap *em_p = new ElevationMap(sr);
    check_allocation(em_p, "em_p", sr);

    Lrp lrp(sr.forced_erp, sr.forced_freq);
    BoundaryFile bf(sr);
    CityFile cf;
    Region region;

    if (!options.ani_filename.empty()) {
        /* read alphanumeric output file from previous simulations if given */

        // TODO: Here's an instance where reading the LRParms may say to load
        // a PAT file but it's never used. Refactor this.
        AntennaPattern pat = AntennaPattern();

        // TODO: Why only the first TX site?
        bool loadPat;
        std::string patFilename;
        lrp.ReadLRParm(tx_site[0], 0, loadPat,
                       patFilename); /* Get ERP status */
        if (loadPat) {
            pat.LoadAntennaPattern(patFilename);
        }
        Anf anf(lrp, sr);

        y = anf.LoadANO(options.ani_filename, sdf, *em_p);

        for (x = 0; x < tx_site.size(); x++)
            em_p->PlaceMarker(tx_site[x]);

        if (sr.rxsite)
            em_p->PlaceMarker(rx_site);

        if (options.boundary_files.size() > 0) {
            for (x = 0; x < options.boundary_files.size(); x++) {
                bf.LoadBoundaries(options.boundary_files[x], *em_p);
            }
            fprintf(stdout, "\n");
            fflush(stdout);
        }

        if (options.city_files.size() > 0) {
            for (x = 0; x < options.city_files.size(); x++) {
                cf.LoadCities(options.city_files[x], *em_p);
            }
            fprintf(stdout, "\n");
            fflush(stdout);
        }

        Image image(sr, options.mapfile, tx_site, *em_p);
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

    min_lon = (int) floor(tx_site[0].lon);
    max_lon = (int) floor(tx_site[0].lon);

    for (y = 0, z = 0; z < tx_site.size(); z++) {
        txlat = (int) floor(tx_site[z].lat);
        txlon = (int) floor(tx_site[z].lon);

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
        rxlat = (int) floor(rx_site.lat);
        rxlon = (int) floor(rx_site.lon);

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

            sr.tx_range =
                sqrt(1.5 * (tx_site[z].alt + em_p->GetElevation(tx_site[z])));

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

            north_min = (int) floor(tx_site[z].lat - sr.deg_range);
            north_max = (int) floor(tx_site[z].lat + sr.deg_range);

            west_min = (int) floor(tx_site[z].lon - sr.deg_range_lon);

            while (west_min < 0)
                west_min += 360;

            while (west_min >= 360)
                west_min -= 360;

            west_max = (int) floor(tx_site[z].lon + sr.deg_range_lon);

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

    if (!options.udt_file.empty()) {
        Udt udt(sr);
        udt.LoadUDT(options.udt_file, *em_p);
    }

    /***** Let the SPLATting begin! *****/

    Report report(*em_p, sr);

    if (sr.pt2pt_mode) {
        em_p->PlaceMarker(rx_site);

        std::string ext;
        if (sr.terrain_plot) {
            /* Extract extension (if present)
             from "terrain_file" */

            ext = Utilities::DivideExtension(options.terrain_file, "png");
        }

        if (sr.elevation_plot) {
            /* Extract extension (if present)
             from "elevation_file" */
            ext = Utilities::DivideExtension(options.elevation_file, "png");
        }

        if (sr.height_plot) {
            /* Extract extension (if present)
             from "height_file" */

            ext = Utilities::DivideExtension(options.height_file, "png");
        }

        if (sr.longley_plot) {
            /* Extract extension (if present)
             from "longley_file" */

            ext = Utilities::DivideExtension(options.longley_file, "png");
        }

        for (x = 0; x < tx_site.size() && x < 4; x++) {
            em_p->PlaceMarker(tx_site[x]);

            if (! sr.nolospath) {
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

            if (! sr.nositereports)
                report.SiteReport(tx_site[x]);

            if (sr.kml) {
                Kml kml(*em_p, sr);
                kml.WriteKML(tx_site[x], rx_site);
            }

            if (sr.kmz) {
                Kml kml(*em_p, sr);
                kml.WriteKMZ(tx_site[x], rx_site);
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
                filename = options.longley_file + oss.str();
                bool longly_file_exists = !options.longley_file.empty();

                bool loadPat;
                std::string patFilename;
                lrp.ReadLRParm(tx_site[x], longly_file_exists, loadPat,
                               patFilename);
                if (loadPat) {
                    pat.LoadAntennaPattern(patFilename);
                }
                report.PathReport(tx_site[x], rx_site, filename,
                                  longly_file_exists, elev, pat, lrp);
            } else {
                bool loadPat;
                std::string patFilename;
                lrp.ReadLRParm(tx_site[x], 1, loadPat, patFilename);
                if (loadPat) {
                    pat.LoadAntennaPattern(patFilename);
                }
                report.PathReport(tx_site[x], rx_site, filename, true, elev,
                                  pat, lrp);
            }

            GnuPlot gnuPlot(sr);

            if (sr.terrain_plot) {
                filename = options.terrain_file + oss.str();
                gnuPlot.GraphTerrain(tx_site[x], rx_site, filename, *em_p);
            }

            if (sr.elevation_plot) {
                filename = options.elevation_file + oss.str();
                gnuPlot.GraphElevation(tx_site[x], rx_site, filename, *em_p);
            }

            if (sr.height_plot) {
                filename = options.height_file + oss.str();
                gnuPlot.GraphHeight(tx_site[x], rx_site, filename,
                                    sr.fresnel_plot, sr.norm, *em_p, lrp);
            }
        }
    }

    if (sr.area_mode && ! sr.topomap) {
        // Allocate the antenna pattern on the heap because it has a huge array
        // of floats that would otherwise be on the stack.
        AntennaPattern *p_pat = new AntennaPattern();
        for (x = 0; x < tx_site.size(); x++) {

            if (sr.coverage) {
                em_p->PlotLOSMap(tx_site[x], sr.altitude);
            } else {
                bool loadPat;
                std::string patFilename;
                char flag = lrp.ReadLRParm(tx_site[x], 1, loadPat, patFilename);
                if (loadPat) {
                    p_pat->LoadAntennaPattern(patFilename);
                }

                if (flag) {
                    em_p->PlotLRMap(tx_site[x], sr.altitudeLR, options.ano_filename,
                                    *p_pat, lrp);
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

        if (options.city_files.size() > 0) {
            CityFile cityFile;

            for (y = 0; y < options.city_files.size(); y++)
                cityFile.LoadCities(options.city_files[y], *em_p);

            fprintf(stdout, "\n");
            fflush(stdout);
        }

        /* Load city and county boundary data files */

        if (options.boundary_files.size() > 0) {
            BoundaryFile boundaryFile(sr);

            for (y = 0; y < options.boundary_files.size(); y++)
                boundaryFile.LoadBoundaries(options.boundary_files[y], *em_p);

            fprintf(stdout, "\n");
            fflush(stdout);
        }

        /* Plot the map */
        Image image(sr, options.mapfile, tx_site, *em_p);
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

    if (sr.command_line_log && !options.logfile.empty()) {
        std::fstream fs;
        fs.open(options.logfile.c_str(), std::fstream::out);

        // TODO: Should we fail silently if we can't open the logfile. Shouldn't
        // we WARN?
        if (fs) {
            for (x = 0; x < (size_t)argc; x++) {
                fs << argv[x] << " ";
            }
            fs << std::endl;
            fs.close();

            std::cout << "\nCommand-line parameter log written to: \""
                      << options.logfile << "\"\n";
        }
    }

    std::cout << std::endl;

    /* json input/output must be somewhere else, but this is how it goes: 
    //================
    arg_t args;
	
	std::string curr_arg = "";
	int curr_arg_i = 0;
	for(int i=0; i<argc; i++) {		// step through argv[] array
		std::string arg = argv[i];
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

void check_allocation(void *ptr, std::string name, const SplatRun &sr) {
    if (ptr == NULL) {
        std::cerr << "\n\a*** ERROR: Could not allocate memory for " << name
                  << " with -maxpages == " << sr.maxpages << "\n\n";
        exit(-1);
    }
}
