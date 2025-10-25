/** @file site.cpp
 *
 * File created by Peter Watkins (KE7IST) 8/12/16.
 * Derived from original project code.
 * Splat!
 * @copyright 1997 - 2018 John A. Magliacane (KD2BD) and contributors.
 * See revision control history for contributions.
 * This file is covered by the LICENSE.md file in the root of this project.
 */

#include "site.h"
#include "utilities.h"
#include <cmath>
#include <cstdio>
#include <string>

Site::Site() { }

Site::Site(const std::string &filename) { LoadQTH(filename); }

double Site::Distance(const Site &site2) const {
    /* This function returns the great circle distance
     in miles between any two site locations. */

    double lat1, lon1, lat2, lon2, distance;

    lat1 = lat * DEG2RAD;
    lon1 = lon * DEG2RAD;
    lat2 = site2.lat * DEG2RAD;
    lon2 = site2.lon * DEG2RAD;

    distance = 3959.0 * acos(sin(lat1) * sin(lat2) +
                             cos(lat1) * cos(lat2) * cos((lon1) - (lon2)));

    return distance;
}

double Site::Azimuth(const Site &destination) const {
    /* This function returns the azimuth (in degrees) to the
     destination as seen from the location of the source. */

    double dest_lat, dest_lon, src_lat, src_lon, beta, azimuth, diff, num, den,
        fraction;

    dest_lat = destination.lat * DEG2RAD;
    dest_lon = destination.lon * DEG2RAD;

    src_lat = lat * DEG2RAD;
    src_lon = lon * DEG2RAD;

    /* Calculate Surface Distance */

    beta = acos(sin(src_lat) * sin(dest_lat) +
                cos(src_lat) * cos(dest_lat) * cos(src_lon - dest_lon));

    /* Calculate Azimuth */

    num = sin(dest_lat) - (sin(src_lat) * cos(beta));
    den = cos(src_lat) * sin(beta);
    fraction = num / den;

    /* Trap potential problems in acos() due to rounding */

    if (fraction >= 1.0)
        fraction = 1.0;

    if (fraction <= -1.0)
        fraction = -1.0;

    /* Calculate azimuth */

    azimuth = acos(fraction);

    /* Reference it to True North */

    diff = dest_lon - src_lon;

    if (diff <= -PI)
        diff += TWOPI;

    if (diff >= PI)
        diff -= TWOPI;

    if (diff > 0.0)
        azimuth = TWOPI - azimuth;

    return (azimuth / DEG2RAD);
}

/**
 This function reads SPLAT! .qth (site location) files.
 The latitude and longitude may be expressed either in
 decimal degrees, or in degree, minute, second format.
 Antenna height is assumed to be expressed in feet above
 ground level (AGL), unless followed by the letter 'M',
 or 'm', or by the word "meters" or "Meters", in which
 case meters is assumed, and is handled accordingly
 */
void Site::LoadQTH(const std::string &filename) {
    /* . */

    std::string qthfile;
    char string[50];
    FILE *fd = NULL;

    size_t x = filename.size();
    qthfile = filename;

    if (qthfile[x - 3] != 'q' || qthfile[x - 2] != 't' ||
        qthfile[x - 1] != 'h') {
        qthfile += ".qth";
    }

    lat = 91.0;
    lon = 361.0;
    alt = 0.0;

    fd = fopen(qthfile.c_str(), "r");

    // Early out if we can't open the file. TODO: Shouldn't we WARN?
    if (fd == NULL)
        return;

    /* Site Name */
    fgets(string, 49, fd);

    /* Strip <CR> and/or <LF> from end of site name */
    name = string;
    Utilities::Chomp(name);

    /* Site Latitude */
    fgets(string, 49, fd);
    lat = Utilities::ReadBearing(string);

    /* Site Longitude */
    fgets(string, 49, fd);
    lon = Utilities::ReadBearing(string);

    if (lon < 0.0)
        lon += 360.0;

    /* Antenna Height */
    fgets(string, 49, fd);
    fclose(fd);

    /* Remove <CR> and/or <LF> from antenna height string */
    for (x = 0; string[x] != 13 && string[x] != 10 && string[x] != 0; x++)
        ;

    string[x] = 0;

    /* Antenna height may either be in feet or meters.
     If the letter 'M' or 'm' is discovered in
     the string, then this is an indication that
     the value given is expressed in meters, and
     must be converted to feet before exiting. */

    for (x = 0;
         string[x] != 'M' && string[x] != 'm' && string[x] != 0 && x < 48; x++)
        ;

    if (string[x] == 'M' || string[x] == 'm') {
        string[x] = 0;
        sscanf(string, "%f", &alt);
        alt *= 3.28084;
    }

    else {
        string[x] = 0;
        sscanf(string, "%f", &alt);
    }

    this->filename = qthfile;
}
