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
#include <fstream>
#include <sstream>
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

    int ypix, xpix, tempxpix, tempypix, fd = 0;
    char tempname[15] = "/tmp/XXXXXX";
    double latitude, longitude, height, tempheight;
    std::string line;
    std::stringstream ss;

    std::ifstream infile(filename);

    if (infile.is_open()) {
        fd = mkstemp(tempname);
        std::ofstream tempfile(tempname);

        fprintf(stdout, "\nReading \"%s\"... ", filename.c_str());
        fflush(stdout);

        while (std::getline(infile, line)) {
            // Strip comments
            size_t comment_pos = line.find(';');
            if (comment_pos != std::string::npos) {
                line = line.substr(0, comment_pos);
            }

            // Parse line for latitude, longitude, height (CSV format)
            size_t first_comma = line.find(',');
            if (first_comma == std::string::npos)
                continue;

            size_t second_comma = line.find(',', first_comma + 1);
            if (second_comma == std::string::npos)
                continue;

            std::string lat_str = line.substr(0, first_comma);
            std::string lon_str =
                line.substr(first_comma + 1, second_comma - first_comma - 1);
            std::string height_str = line.substr(second_comma + 1);

            latitude = Utilities::ReadBearing(lat_str);
            longitude = Utilities::ReadBearing(lon_str);

            if (longitude < 0.0)
                longitude += 360.0;

            /* The terrain feature may be expressed in either
             feet or meters.  If the letter 'M' or 'm' is
             discovered in the string, then this is an
             indication that the value given is expressed
             in meters.  Otherwise the height is interpreted
             as being expressed in feet.  */

            if (height_str.find('M') != std::string::npos ||
                height_str.find('m') != std::string::npos) {
                // Remove 'M' or 'm'
                size_t m_pos = height_str.find_first_of("Mm");
                if (m_pos != std::string::npos) {
                    height_str = height_str.substr(0, m_pos);
                }
                height = rint(std::stod(height_str));
            } else {
                height = rint(METERS_PER_FOOT * std::stod(height_str));
            }

            if (height > 0.0)
                tempfile << (int) rint(latitude / sr.dpp) << ", "
                         << (int) rint(longitude / sr.dpp) << ", " << height
                         << "\n";
        }

        infile.close();
        tempfile.close();
        close(fd);

        fprintf(stdout, "Done!");
        fflush(stdout);

        std::ifstream tempfile1(tempname);
        int y = 0;
        int x, z;
        char comma;

        // Read first line from tempfile1
        if (std::getline(tempfile1, line)) {
            ss.clear();
            ss.str(line);
            ss >> xpix >> comma >> ypix >> comma >> height;

            while (std::getline(tempfile1, line)) {
                x = 0;
                z = 0;

                std::ifstream tempfile2(tempname);
                std::string line2;

                // Read first line from tempfile2
                if (std::getline(tempfile2, line2)) {
                    ss.clear();
                    ss.str(line2);
                    ss >> tempxpix >> comma >> tempypix >> comma >> tempheight;

                    while (z == 0 && std::getline(tempfile2, line2)) {
                        if (x > y && xpix == tempxpix && ypix == tempypix) {
                            z = 1; /* Dupe! */

                            if (tempheight > height)
                                height = tempheight;
                        } else {
                            ss.clear();
                            ss.str(line2);
                            ss >> tempxpix >> comma >> tempypix >> comma >>
                                tempheight;
                            x++;
                        }
                    }
                }
                tempfile2.close();

                if (z == 0) /* No duplicate found */
                    em.AddElevation(xpix * sr.dpp, ypix * sr.dpp, height);

                // Parse next line for xpix, ypix, height
                ss.clear();
                ss.str(line);
                ss >> xpix >> comma >> ypix >> comma >> height;
                y++;
            }
        }

        tempfile1.close();
        unlink(tempname);
    }

    else
        fprintf(stderr, "\n*** ERROR: \"%s\": not found!", filename.c_str());

    fprintf(stdout, "\n");
}
