/****************************************************************************
*                CITYDECODER: A SPLAT! File Conversion Utility              *
*                  Copyright John A. Magliacane, KD2BD 2002                 *
*                         Last update: 20-May-2014                          *
*****************************************************************************
*                                                                           *
* This utility reads ASCII Metadata Cartographic Boundary Files available   *
* through the U.S. Census Bureau, and generates a lists of cities, states,  *
* counties, and county subdivisions along with the latitude and longitude   *
* corresponding to their geographic centers.  Such data may be (optionally) *
* sorted and written to files for use with SPLAT! software.  This utility   *
* takes as an argument, a two-letter prefix plus the FIPS code for the      *
* state being processed (ie: "citydecoder pl34" will read files             *
* "pl34_d00.dat" and "pl34_d00a.dat", and "citydecoder cs34" will read      *
* files "cs34_d00.dat" and "cs34_d00a.dat", and produce a list of city      *
* names and geographical coordinates for the state of New Jersey.           *
*                                                                           *
* ZIP compressed data files for the United States may be downloaded from:   *
*                                                                           *
* http://web.archive.org/web/20130331172800/http://www.census.gov/geo/www/cob/cs2000.html *
*                                                                           *
* Please select among the ARC/INFO Ungenerate (ASCII) formatted files at    *
* the bottom of each page, and use "unzip -a" or "gunzip" to properly       *
* unzip these files under Unix/Linux prior to use.                          *
*                                                                           *
*****************************************************************************
*                                                                           *
* This program is free software; you can redistribute it and/or modify it   *
* under the terms of the GNU General Public License as published by the     *
* Free Software Foundation; either version 2 of the License or any later    *
* version.                                                                  *
*                                                                           *
* This program is distributed in the hope that it will useful, but WITHOUT  *
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or     *
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License     *
* for more details.                                                         *
*                                                                           *
*****************************************************************************
*          To compile: gcc -Wall -O3 -s citydecoder.c -o citydecoder        *
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(argc, argv)
char argc, *argv[];
{
    int x, y, z;
    long attributefile_id, coordinatefile_id;
    char string[80], name[80], attributefilename[15], coordinatefilename[15];
    double lat, lon;
    FILE *attributefile = NULL, *coordinatefile = NULL;

    if (argc == 1) {
        fprintf(stderr, "\n*** Usage: citydecoder pl34 cs34 pl42 cs42 | sort > "
                        "outputfile\n\n");
        exit(1);
    }

    for (z = 1; z < argc; z++) {
        sprintf(attributefilename, "%s_d00a.dat", argv[z]);
        sprintf(coordinatefilename, "%s_d00.dat", argv[z]);

        attributefile = fopen(attributefilename, "r");
        coordinatefile = fopen(coordinatefilename, "r");

        if (attributefile != NULL && coordinatefile != NULL) {
            /* Skip First ASCII File Record (ID=0) */

            for (x = 0; x < 7; x++) {
                if (fgets(string, 80, attributefile) == NULL) {
                    fprintf(
                        stderr,
                        "*** Error reading attribute file header at line %d\n",
                        x + 1);
                    fclose(attributefile);
                    fclose(coordinatefile);
                    continue; /* Skip to next file */
                }
            }

            /* Skip yet another line for "cs" files */

            if (argv[z][0] == 'c' && argv[z][1] == 's') {
                if (fgets(string, 80, attributefile) == NULL) {
                    fprintf(
                        stderr,
                        "*** Error reading attribute file extra header line\n");
                    fclose(attributefile);
                    fclose(coordinatefile);
                    continue; /* Skip to next file */
                }
            }
            do {
                string[0] = 0;
                if (fscanf(coordinatefile, "%ld", &coordinatefile_id) != 1) {
                    if (feof(coordinatefile))
                        break; /* End of file reached normally */
                    fprintf(stderr, "*** Error reading coordinate file ID\n");
                    break;
                }

                if (coordinatefile_id != -99999) {
                    name[0] = 0;

                    if (fscanf(coordinatefile, "%lf %lf", &lon, &lat) != 2) {
                        fprintf(stderr,
                                "*** Error reading coordinates (lon, lat)\n");
                        break;
                    }

                    /* Read ID Number From Attribute File */

                    if (fgets(string, 80, attributefile) == NULL) {
                        fprintf(stderr,
                                "*** Error reading attribute file ID line\n");
                        break;
                    }
                    if (sscanf(string, "%ld", &attributefile_id) != 1) {
                        fprintf(stderr,
                                "*** Error parsing attribute file ID\n");
                        continue; /* Try next record */
                    }

                    /* Skip Several Strings in Attribute File */

                    if (fgets(string, 80, attributefile) == NULL ||
                        fgets(string, 80, attributefile) == NULL) {
                        fprintf(
                            stderr,
                            "*** Error reading attribute file data lines\n");
                        break;
                    }

                    /* Skip a third line for "cs" files */

                    if (argv[z][0] == 'c' && argv[z][1] == 's') {
                        if (fgets(string, 80, attributefile) == NULL) {
                            fprintf(
                                stderr,
                                "*** Error reading attribute file cs line\n");
                            break;
                        }
                    }

                    /* Read City Name From Attribute File */

                    if (fgets(string, 80, attributefile) == NULL) {
                        fprintf(stderr, "*** Error reading city name from "
                                        "attribute file\n");
                        break;
                    }

                    /* Strip "quote" characters from name */

                    for (x = 2, y = 0; string[x] != '"' && string[x] != 0;
                         x++, y++)
                        name[y] = string[x];

                    name[y] = 0;

                    /* Skip Two Strings in Attribute File */

                    if (fgets(string, 80, attributefile) == NULL ||
                        fgets(string, 80, attributefile) == NULL) {
                        fprintf(
                            stderr,
                            "*** Error reading attribute file tail lines\n");
                        break;
                    }

                    /* Skip blank line between records */

                    if (fgets(string, 80, attributefile) == NULL) {
                        /* It's OK if this fails at end of file */
                        if (! feof(attributefile))
                            fprintf(stderr, "*** Error reading attribute file "
                                            "blank line\n");
                    }

                    if (name[0] != 0 && name[0] != ' ' &&
                        feof(attributefile) == 0 &&
                        attributefile_id == coordinatefile_id)
                        printf("%s, %f, %f\n", name, lat, -lon);
                }

                /* Read to the end of the current coordinatefile record */

                do {
                    string[0] = 0;
                    if (fscanf(coordinatefile, "%80s", string) != 1) {
                        if (! feof(coordinatefile))
                            fprintf(stderr, "*** Error reading coordinate file "
                                            "END marker\n");
                        break;
                    }

                } while (strncmp(string, "END", 3) != 0 &&
                         feof(coordinatefile) == 0);

            } while (feof(coordinatefile) == 0);

            fclose(attributefile);
            fclose(coordinatefile);
        }

        else {
            /* Houston, We Have A Problem... */

            fprintf(stderr, "%c\n", 7);

            if (coordinatefile == NULL)
                fprintf(stderr, "*** Error opening coordinate file: \"%s\"!\n",
                        coordinatefilename);

            if (attributefile == NULL)
                fprintf(stderr, "*** Error opening attribute file : \"%s\"!\n",
                        attributefilename);
            fprintf(stderr, "\n");
        }
    }

    exit(0);
}
