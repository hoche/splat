/** @file path.cpp
 *
 * File created by Peter Watkins (KE7IST) 4/8/16.
 * Derived from original project code.
 * Splat!
 * @copyright 1997 - 2018 John A. Magliacane (KD2BD) and contributors.
 * See revision control history for contributions.
 * This file is covered by the LICENSE.md file in the root of this project.
 */

#include "path.h"
#include "dem.h"
#include "elevation_map.h"
#include "lrp.h"
#include "antenna_pattern.h"
#include "sdf.h"
#include "site.h"
#include "splat_run.h"
#include "utilities.h"
#include <bzlib.h>
#include <cmath>
#include <string>
#include <vector>

void Path::ReadPath(const Site &source, const Site &destination,
                    const ElevationMap &em) {
    /* This function generates a sequence of latitude and
     longitude positions between source and destination
     locations along a great circle path, and stores
     elevation and distance information for points
     along that path in the "path" structure. */

    int c;
    double azimuth, distance_scalar, lat1, lon1, beta, den, num, lat2, lon2,
        total_distance, dx, dy, path_length, miles_per_sample,
        samples_per_radian = 68755.0;
    // struct	site tempsite; let's try using the Site object instead
    Site tempsite;

    lat1 = source.lat * DEG2RAD;
    lon1 = source.lon * DEG2RAD;

    lat2 = destination.lat * DEG2RAD;
    lon2 = destination.lon * DEG2RAD;

    if (ppd == 1200.0)
        samples_per_radian = 68755.0;

    if (ppd == 3600.0)
        samples_per_radian = 206265.0;

    azimuth = source.Azimuth(destination) * DEG2RAD;

    total_distance = source.Distance(destination);

    if (total_distance > (30.0 / ppd)) /* > 0.5 pixel distance */
    {
        dx = samples_per_radian * acos(cos(lon1 - lon2));
        dy = samples_per_radian * acos(cos(lat1 - lat2));

        path_length = sqrt((dx * dx) + (dy * dy)); /* Total number of samples */

        miles_per_sample = total_distance / path_length; /* Miles per sample */
    }

    else {
        c = 0;
        dx = 0.0;
        dy = 0.0;
        path_length = 0.0;
        miles_per_sample = 0.0;
        total_distance = 0.0;

        lat1 = lat1 / DEG2RAD;
        lon1 = lon1 / DEG2RAD;

        lat[c] = lat1;
        lon[c] = lon1;
        elevation[c] = em.GetElevation(source);
        distance[c] = 0.0;
    }

    for (distance_scalar = 0.0, c = 0;
         (total_distance != 0.0 && distance_scalar <= total_distance &&
          c < arraysize);
         c++, distance_scalar = miles_per_sample * (double)c) {
        beta = distance_scalar / 3959.0;
        lat2 =
            asin(sin(lat1) * cos(beta) + cos(azimuth) * sin(beta) * cos(lat1));
        num = cos(beta) - (sin(lat1) * sin(lat2));
        den = cos(lat1) * cos(lat2);

        if (azimuth == 0.0 && (beta > HALFPI - lat1))
            lon2 = lon1 + PI;

        else if (azimuth == HALFPI && (beta > HALFPI + lat1))
            lon2 = lon1 + PI;

        else if (fabs(num / den) > 1.0)
            lon2 = lon1;

        else {
            if ((PI - azimuth) >= 0.0)
                lon2 = lon1 - Utilities::arccos(num, den);
            else
                lon2 = lon1 + Utilities::arccos(num, den);
        }

        while (lon2 < 0.0)
            lon2 += TWOPI;

        while (lon2 > TWOPI)
            lon2 -= TWOPI;

        lat2 = lat2 / DEG2RAD;
        lon2 = lon2 / DEG2RAD;

        // TODO: Sometimes we get EXEC_BAD_ACCESS here
        lat[c] = lat2;
        lon[c] = lon2;
        tempsite.lat = lat2;
        tempsite.lon = lon2;
        elevation[c] = em.GetElevation(tempsite);
        distance[c] = distance_scalar;
    }

    /* Make sure exact destination point is recorded at length-1 */

    if (c < arraysize) {
        lat[c] = destination.lat;
        lon[c] = destination.lon;
        elevation[c] = em.GetElevation(destination);
        distance[c] = total_distance;
        c++;
    }

    if (c < arraysize)
        length = c;
    else
        length = arraysize - 1;
}

Path::~Path() {}
