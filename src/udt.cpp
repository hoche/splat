/** @file udt.cpp
 *
 * File created by Peter Watkins (KE7IST) 1/9/18.
 * Derived from original project code.
 * Splat!
 * @copyright 1997 - 2018 John A. Magliacane (KD2BD) and contributors.
 * See revision control history for contributions.
 * This file is covered by the LICENSE.md file in the root of this project.
 */

#include "udt.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unistd.h>

#include "elevation_map.h"
#include "splat_run.h"
#include "utilities.h"

void Udt::LoadUDT(const std::string &filename, ElevationMap &em) {
    /* This function reads a file containing User-Defined Terrain
     features for their addition to the digital elevation model
     data used by SPLAT!.  Elevations in the UDT file are evaluated
     and then copied into a temporary file under /tmp.  Then the
     contents of the temp file are scanned, and if found to be unique,
     are added to the ground elevations described by the digital
     elevation data already loaded into memory. */

    int i, x, y, z, ypix, xpix, tempxpix, tempypix, fd = 0;
    char input[80], str[3][80], tempname[15] = "/tmp/XXXXXX", *pointer = NULL;
    double latitude, longitude, height, tempheight;
    FILE *fd1 = NULL, *fd2 = NULL;

    fd1 = fopen(filename.c_str(), "r");

    if (fd1 != NULL) {
        fd = mkstemp(tempname);
        fd2 = fopen(tempname, "w");

        fgets(input, 78, fd1);

        pointer = strchr(input, ';');

        if (pointer != NULL)
            *pointer = 0;

        fprintf(stdout, "\nReading \"%s\"... ", filename.c_str());
        fflush(stdout);

        while (feof(fd1) == 0) {
            /* Parse line for latitude, longitude, height */

            for (x = 0, y = 0, z = 0; x < 78 && input[x] != 0 && z < 3; x++) {
                if (input[x] != ',' && y < 78) {
                    str[z][y] = input[x];
                    y++;
                }

                else {
                    str[z][y] = 0;
                    z++;
                    y = 0;
                }
            }

            latitude = Utilities::ReadBearing(str[0]);
            longitude = Utilities::ReadBearing(str[1]);

            if (longitude < 0.0)
                longitude += 360.0;

            /* Remove <CR> and/or <LF> from antenna height string */

            for (i = 0; str[2][i] != 13 && str[2][i] != 10 && str[2][i] != 0;
                 i++)
                ;

            str[2][i] = 0;

            /* The terrain feature may be expressed in either
             feet or meters.  If the letter 'M' or 'm' is
             discovered in the string, then this is an
             indication that the value given is expressed
             in meters.  Otherwise the height is interpreted
             as being expressed in feet.  */

            for (i = 0; str[2][i] != 'M' && str[2][i] != 'm' &&
                        str[2][i] != 0 && i < 48;
                 i++)
                ;

            if (str[2][i] == 'M' || str[2][i] == 'm') {
                str[2][i] = 0;
                height = rint(atof(str[2]));
            }

            else {
                str[2][i] = 0;
                height = rint(METERS_PER_FOOT * atof(str[2]));
            }

            if (height > 0.0)
                fprintf(fd2, "%d, %d, %f\n", (int)rint(latitude / sr.dpp),
                        (int)rint(longitude / sr.dpp), height);

            fgets(input, 78, fd1);

            pointer = strchr(input, ';');

            if (pointer != NULL)
                *pointer = 0;
        }

        fclose(fd1);
        fclose(fd2);
        close(fd);

        fprintf(stdout, "Done!");
        fflush(stdout);

        fd1 = fopen(tempname, "r");
        fd2 = fopen(tempname, "r");

        y = 0;

        fscanf(fd1, "%d, %d, %lf", &xpix, &ypix, &height);

        do {
            x = 0;
            z = 0;

            fscanf(fd2, "%d, %d, %lf", &tempxpix, &tempypix, &tempheight);

            do {
                if (x > y && xpix == tempxpix && ypix == tempypix) {
                    z = 1; /* Dupe! */

                    if (tempheight > height)
                        height = tempheight;
                }

                else {
                    fscanf(fd2, "%d, %d, %lf", &tempxpix, &tempypix,
                           &tempheight);
                    x++;
                }

            } while (feof(fd2) == 0 && z == 0);

            if (z == 0) /* No duplicate found */
                em.AddElevation(xpix * sr.dpp, ypix * sr.dpp, height);

            fscanf(fd1, "%d, %d, %lf", &xpix, &ypix, &height);
            y++;

            rewind(fd2);

        } while (feof(fd1) == 0);

        fclose(fd1);
        fclose(fd2);
        unlink(tempname);
    }

    else
        fprintf(stderr, "\n*** ERROR: \"%s\": not found!", filename.c_str());

    fprintf(stdout, "\n");
}
