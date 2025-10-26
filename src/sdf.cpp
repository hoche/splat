/** @file sdf.cpp
 *
 * File created by Peter Watkins (KE7IST) 1/8/18.
 * Derived from original project code.
 * Splat!
 * @copyright 1997 - 2018 John A. Magliacane (KD2BD) and contributors.
 * See revision control history for contributions.
 * This file is covered by the LICENSE.md file in the root of this project.
 */

#include "sdf.h"
#include "antenna_pattern.h"
#include "dem.h"
#include "elevation_map.h"
#include "lrp.h"
#include "path.h"
#include "sdf_bz.h"
#include "site.h"
#include "splat_run.h"
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

/// This function reads a SPLAT Data File containing digital elevation model
/// data into memory.  Elevation data, maximum and minimum elevations, and
/// quadrangle limits are stored in the first available em.dem[] structure. The
/// caller must provide minimum and maximum lattitude and longitude values.
/// These determine the DEM this function picks. If the DEM already exists in
/// the Elevation Map, this function exits. It also exits if there are no more
/// DEM pages available into which the data may be loaded.
/// @param em The elevation map into which to load the SDF data
/// @param name The name of the SDF without the filename suffix
/// @param minlat The minimum lattitude value
/// @param maxlat The maximum lattitude value
/// @param minlon The minimum longitude value
/// @param maxlon The maximum longitude value
int Sdf::LoadSDF(ElevationMap &em, const std::string &name, int minlat,
                 int maxlat, int minlon, int maxlon) {
    int x, y, data, indx;
    std::stringstream ss;

    std::string path_plus_name;
    std::string sdf_file = name + suffix;

    Dem *dem = FindEmptyDem(em, minlat, maxlat, minlon, maxlon, indx);

    // Early-out if there's no need to load the file
    if (dem == NULL) {
        return 0;
    }

    /* Search for SDF file in current working directory first */
    path_plus_name = sdf_file;
    if (! OpenFile(path_plus_name)) {
        /* Next, try loading SDF file from path specified
         in $HOME/.splat_path file or by -d argument */
        path_plus_name = sdf_path + sdf_file;

        // Stop here if the file couldn't be opened
        if (! OpenFile(path_plus_name)) {
            return -1;
        }
    }

    fprintf(stdout, "Loading \"%s\" into page %d...", path_plus_name.c_str(),
            indx + 1);
    fflush(stdout);

    // Read header values
    if (GetString()) {
        ss.clear();
        ss.str(line);
        ss >> dem->max_west;
    }
    if (GetString()) {
        ss.clear();
        ss.str(line);
        ss >> dem->min_north;
    }
    if (GetString()) {
        ss.clear();
        ss.str(line);
        ss >> dem->min_west;
    }
    if (GetString()) {
        ss.clear();
        ss.str(line);
        ss >> dem->max_north;
    }

    for (x = 0; x < sr.ippd; x++) {
        for (y = 0; y < sr.ippd; y++) {
            if (GetString()) {
                ss.clear();
                ss.str(line);
                ss >> data;

                dem->data[x * sr.ippd + y] = data;
                dem->signal[x * sr.ippd + y] = 0;
                dem->mask[x * sr.ippd + y] = 0;

                if (data > dem->max_el)
                    dem->max_el = data;

                if (data < dem->min_el)
                    dem->min_el = data;
            }
        }
    }

    CloseFile();

    if (dem->min_el < em.min_elevation)
        em.min_elevation = dem->min_el;

    if (dem->max_el > em.max_elevation)
        em.max_elevation = dem->max_el;

    if (em.max_north == -90) {
        em.max_north = dem->max_north;
    } else if (dem->max_north > em.max_north) {
        em.max_north = dem->max_north;
    }

    if (em.min_north == 90) {
        em.min_north = dem->min_north;
    } else if (dem->min_north < em.min_north) {
        em.min_north = dem->min_north;
    }

    if (em.max_west == -1) {
        em.max_west = dem->max_west;
    } else {
        if (abs(dem->max_west - em.max_west) < 180) {
            if (dem->max_west > em.max_west)
                em.max_west = dem->max_west;
        } else {
            if (dem->max_west < em.max_west)
                em.max_west = dem->max_west;
        }
    }

    if (em.min_west == 360) {
        em.min_west = dem->min_west;
    } else {
        if (abs(dem->min_west - em.min_west) < 180) {
            if (dem->min_west < em.min_west)
                em.min_west = dem->min_west;
        } else {
            if (dem->min_west > em.min_west)
                em.min_west = dem->min_west;
        }
    }

    fprintf(stdout, " Done!\n");
    fflush(stdout);

    return 1;
}

/// This function loads the requested SDF file from the filesystem. It first
/// tries to load an uncompressed SDF file (since uncompressed files load
/// slightly faster).  If that attempt fails, then it tries to load a compressed
/// SDF file. If that fails, then we can assume that no elevation data exists
/// for the region requested, and that the region requested must be entirely
/// over water. In addition, this function will load HD SDF files (using the
/// appropriate filename particle) if Splat! is configured to run in HD mode.
/// The caller must provide minimum and maximum lattitude and longitude values.
/// These determine the DEM this function picks. If the DEM already exists in
/// the Elevation Map, this function exits. It also exits if there are no more
/// DEM pages available into which the data may be loaded.
/// @param em The elevation map into which to load the SDF data
/// @param minlat The minimum lattitude value
/// @param maxlat The maximum lattitude value
/// @param minlon The minimum longitude value
/// @param maxlon The maximum longitude value
char Sdf::LoadSDF(ElevationMap &em, int minlat, int maxlat, int minlon,
                  int maxlon) {
    int x, y, indx;
    int return_value = -1;
    std::string name = "" + std::to_string(minlat) + sr.sdf_delimiter +
                       std::to_string(maxlat) + sr.sdf_delimiter +
                       std::to_string(minlon) + sr.sdf_delimiter +
                       std::to_string(maxlon) + (sr.hd_mode ? "-hd" : "");

    // Try to load an uncompressed SDF first.
    // Stop here if we successfully loaded the file
    if (LoadSDF(em, name, minlat, maxlat, minlon, maxlon) == 1) {
        return return_value;
    }

    // Try loading a compressed SDF.
    // Stop here if we successfully loaded the file
    SdfBz sdfBz = SdfBz(sdf_path, sr);
    if (sdfBz.LoadSDF(em, name, minlat, maxlat, minlon, maxlon) == 1) {
        return return_value;
    }

    // If neither format can be found, then assume the area is water.
    // Is it already in memory?
    Dem *dem = FindEmptyDem(em, minlat, maxlat, minlon, maxlon, indx);

    // Early-out if there's no need to load the data
    if (dem == NULL) {
        return 0;
    }

    fprintf(stdout, "Region  \"%s\" assumed as sea-level into page %d...",
            name.c_str(), indx + 1);
    fflush(stdout);

    dem->max_west = maxlon;
    dem->min_north = minlat;
    dem->min_west = minlon;
    dem->max_north = maxlat;

    /* Fill DEM with sea-level topography */

    for (x = 0; x < sr.ippd; x++) {
        for (y = 0; y < sr.ippd; y++) {
            dem->data[x * sr.ippd + y] = 0;
            dem->signal[x * sr.ippd + y] = 0;
            dem->mask[x * sr.ippd + y] = 0;

            if (dem->min_el > 0)
                dem->min_el = 0;
        }
    }

    if (dem->min_el < em.min_elevation)
        em.min_elevation = dem->min_el;

    if (dem->max_el > em.max_elevation)
        em.max_elevation = dem->max_el;

    if (em.max_north == -90) {
        em.max_north = dem->max_north;
    } else if (dem->max_north > em.max_north) {
        em.max_north = dem->max_north;
    }

    if (em.min_north == 90) {
        em.min_north = dem->min_north;
    } else if (dem->min_north < em.min_north) {
        em.min_north = dem->min_north;
    }

    if (em.max_west == -1) {
        em.max_west = dem->max_west;
    } else {
        if (abs(dem->max_west - em.max_west) < 180) {
            if (dem->max_west > em.max_west)
                em.max_west = dem->max_west;
        } else {
            if (dem->max_west < em.max_west)
                em.max_west = dem->max_west;
        }
    }

    if (em.min_west == 360) {
        em.min_west = dem->min_west;
    } else {
        if (abs(dem->min_west - em.min_west) < 180) {
            if (dem->min_west < em.min_west)
                em.min_west = dem->min_west;
        } else {
            if (dem->min_west > em.min_west)
                em.min_west = dem->min_west;
        }
    }

    fprintf(stdout, " Done!\n");
    fflush(stdout);

    return 1;
}

/// Returns the DEM matching the given coordinates. Returns NULL if the DEM
/// couldn't be found or it isn't empty.
/// @param em The Elevation Map in which to look for an empty DEM
/// @param minlat The minimum lattitude value
/// @param maxlat The maximum lattitude value
/// @param minlon The minimum longitude value
/// @param maxlon The maximum longitude value
/// @param indx The index of the found DEM.
Dem *Sdf::FindEmptyDem(ElevationMap &em, int minlat, int maxlat, int minlon,
                       int maxlon, int &indx) {
    for (int i = 0; i < sr.maxpages; i++) {
        if (minlat == em.dem[i].min_north && minlon == em.dem[i].min_west &&
            maxlat == em.dem[i].max_north && maxlon == em.dem[i].max_west) {
            // It already exists. It's not empty
            return NULL;
        }
    }

    // Is room available to load it?
    for (int i = 0; i < sr.maxpages; i++) {
        if (em.dem[i].max_north == -90) {
            indx = i;
            return &em.dem[i];
        }
    }

    // There wasn't a free page
    return NULL;
}

bool Sdf::GetString() { return static_cast<bool>(std::getline(infile, line)); }

bool Sdf::OpenFile(std::string path) {
    infile.open(path);
    return infile.is_open();
}

void Sdf::CloseFile() { infile.close(); }
