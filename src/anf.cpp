/** @file anf.cpp
 *
 * File created by Peter Watkins (KE7IST) 1/9/18.
 * Derived from original project code.
 * Splat!
 * @copyright 1997 - 2018 John A. Magliacane (KD2BD) and contributors.
 * See revision control history for contributions.
 * This file is covered by the LICENSE.md file in the root of this project.
 */

#include "anf.h"
#include "elevation_map.h"
#include "lrp.h"
#include "sdf.h"
#include "splat_run.h"
#include <cmath>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>

/// This function reads a SPLAT! alphanumeric output file (-ani option) for
/// analysis and/or map generation.
/// @param filename The path to the alphanumeric output file
/// @param sdf The SDF object
/// @param em The elevation map
int Anf::LoadANO(const std::string &filename, Sdf &sdf, ElevationMap &em) {
    int error = 0, max_west, min_west, max_north, min_north;
    std::string line;
    double latitude = 0.0, longitude = 0.0, azimuth = 0.0, elevation = 0.0,
           ano = 0.0;
    std::stringstream ss;

    std::ifstream infile(filename);

    if (infile.is_open()) {
        // Read and parse first line: max_west, min_west
        if (std::getline(infile, line)) {
            size_t comment_pos = line.find(';');
            if (comment_pos != std::string::npos) {
                line = line.substr(0, comment_pos);
            }
            ss.clear();
            ss.str(line);
            char comma;
            ss >> max_west >> comma >> min_west;
        }

        // Read and parse second line: max_north, min_north
        if (std::getline(infile, line)) {
            size_t comment_pos = line.find(';');
            if (comment_pos != std::string::npos) {
                line = line.substr(0, comment_pos);
            }
            ss.clear();
            ss.str(line);
            char comma;
            ss >> max_north >> comma >> min_north;
        }

        // Skip third line
        std::getline(infile, line);

        em.LoadTopoData(max_west - 1, min_west, max_north - 1, min_north, sdf);

        fprintf(stdout, "\nReading \"%s\"... ", filename.c_str());
        fflush(stdout);

        // Read first data line
        if (std::getline(infile, line)) {
            ss.clear();
            ss.str(line);
            char comma;
            ss >> latitude >> comma >> longitude >> comma >> azimuth >> comma >>
                elevation >> comma >> ano;
        }

        while (infile.good()) {
            if (lrp.erp == 0.0) {
                /* Path loss */

                if (sr.contour_threshold == 0 ||
                    (fabs(ano) <= (double) sr.contour_threshold)) {
                    ano = fabs(ano);

                    if (ano > 255.0)
                        ano = 255.0;

                    em.PutSignal(latitude, longitude,
                                 ((unsigned char) round(ano)));
                }
            }

            if (lrp.erp != 0.0 && sr.dbm) {
                /* signal power level in dBm */

                if (sr.contour_threshold == 0 ||
                    (ano >= (double) sr.contour_threshold)) {
                    ano = 200.0 + rint(ano);

                    if (ano < 0.0)
                        ano = 0.0;

                    if (ano > 255.0)
                        ano = 255.0;

                    em.PutSignal(latitude, longitude,
                                 ((unsigned char) round(ano)));
                }
            }

            if (lrp.erp != 0.0 && ! sr.dbm) {
                /* field strength dBuV/m */

                if (sr.contour_threshold == 0 ||
                    (ano >= (double) sr.contour_threshold)) {
                    ano = 100.0 + rint(ano);

                    if (ano < 0.0)
                        ano = 0.0;

                    if (ano > 255.0)
                        ano = 255.0;

                    em.PutSignal(latitude, longitude,
                                 ((unsigned char) round(ano)));
                }
            }

            // Read next line
            if (std::getline(infile, line)) {
                ss.clear();
                ss.str(line);
                char comma;
                ss >> latitude >> comma >> longitude >> comma >> azimuth >>
                    comma >> elevation >> comma >> ano;
            }
        }

        infile.close();

        fprintf(stdout, " Done!\n");
        fflush(stdout);
    } else
        error = 1;

    return error;
}
