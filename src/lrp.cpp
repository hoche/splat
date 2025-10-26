/** @file lrp.cpp
 *
 * File created by Peter Watkins (KE7IST) 1/8/18.
 * Derived from original project code.
 * Splat!
 * @copyright 1997 - 2018 John A. Magliacane (KD2BD) and contributors.
 * See revision control history for contributions.
 * This file is covered by the LICENSE.md file in the root of this project.
 */

#include "lrp.h"
#include "site.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

char Lrp::ReadLRParm(const Site &txsite, char forced_read, bool &loadPat,
                     std::string &filenameOut) {
    /* This function reads ITM parameter data for the transmitter
     site.  The file name is the same as the txsite, except the
     filename extension is .lrp.  If the needed file is not found,
     then the file "splat.lrp" is read from the current working
     directory.  Failure to load this file under a forced_read
     condition will result in the default parameters hard coded
     into this function to be used and written to "splat.lrp". */

    double din;
    std::string filename;
    std::string line;
    char return_value = 0;
    int iin = 0;
    bool ok = true;
    FILE *outfile = NULL;

    // Helper lambda to read line and strip comments
    auto read_and_strip = [](std::ifstream &file, std::string &line) -> bool {
        if (std::getline(file, line)) {
            // Strip comments (everything after ';')
            size_t comment_pos = line.find(';');
            if (comment_pos != std::string::npos) {
                line = line.substr(0, comment_pos);
            }
            return true;
        }
        return false;
    };

    /* Default parameters */

    eps_dielect = 0.0;
    sgm_conductivity = 0.0;
    eno_ns_surfref = 0.0;
    frq_mhz = 0.0;
    radio_climate = 0;
    pol = 0;
    conf = 0.0;
    rel = 0.0;
    erp = 0.0;

    /* Generate .lrp filename from txsite filename. */

    std::string base = txsite.filename;
    size_t dot_pos = base.find('.');
    if (dot_pos != std::string::npos) {
        filename = base.substr(0, dot_pos) + ".lrp";
    } else {
        filename = base + ".lrp";
    }

    std::ifstream infile(filename);

    if (! infile.is_open()) {
        /* Load default "splat.lrp" file */
        filename = "splat.lrp";
        infile.open(filename);
    }

    if (infile.is_open()) {
        std::stringstream ss;

        // Read eps_dielect
        if (read_and_strip(infile, line)) {
            ss.clear();
            ss.str(line);
            if (ss >> din) {
                eps_dielect = din;
            } else {
                ok = false;
            }
        } else {
            ok = false;
        }

        // Read sgm_conductivity
        if (ok && read_and_strip(infile, line)) {
            ss.clear();
            ss.str(line);
            if (ss >> din) {
                sgm_conductivity = din;
            } else {
                ok = false;
            }
        } else {
            ok = false;
        }

        // Read eno_ns_surfref
        if (ok && read_and_strip(infile, line)) {
            ss.clear();
            ss.str(line);
            if (ss >> din) {
                eno_ns_surfref = din;
            } else {
                ok = false;
            }
        } else {
            ok = false;
        }

        // Read frq_mhz
        if (ok && read_and_strip(infile, line)) {
            ss.clear();
            ss.str(line);
            if (ss >> din) {
                frq_mhz = din;
            } else {
                ok = false;
            }
        } else {
            ok = false;
        }

        // Read radio_climate
        if (ok && read_and_strip(infile, line)) {
            ss.clear();
            ss.str(line);
            if (ss >> iin) {
                radio_climate = iin;
            } else {
                ok = false;
            }
        } else {
            ok = false;
        }

        // Read pol
        if (ok && read_and_strip(infile, line)) {
            ss.clear();
            ss.str(line);
            if (ss >> iin) {
                pol = iin;
            } else {
                ok = false;
            }
        } else {
            ok = false;
        }

        // Read conf
        if (ok && read_and_strip(infile, line)) {
            ss.clear();
            ss.str(line);
            if (ss >> din) {
                conf = din;
            } else {
                ok = false;
            }
        } else {
            ok = false;
        }

        // Read rel
        if (ok && read_and_strip(infile, line)) {
            ss.clear();
            ss.str(line);
            if (ss >> din) {
                rel = din;
                return_value = 1;
            } else {
                ok = false;
            }
        } else {
            ok = false;
        }

        // Read erp (optional)
        if (ok && read_and_strip(infile, line)) {
            ss.clear();
            ss.str(line);
            if (ss >> din) {
                erp = din;

                /* ERP in SPLAT! is referenced to 1 Watt
                 into a dipole (0 dBd).  If ERP is
                 expressed in dBm (referenced to a
                 0 dBi radiator), convert dBm in EIRP
                 to ERP.  */

                if (line.find("dBm") != std::string::npos ||
                    line.find("dbm") != std::string::npos) {
                    erp = (pow(10.0, (erp - 32.14) / 10.0));
                }
            }
        }

        infile.close();

        if (forced_erp != -1.0)
            erp = forced_erp;

        if (forced_freq >= 20.0 && forced_freq <= 20000.0)
            frq_mhz = forced_freq;

        if (ok) {
            filenameOut = filename;
            loadPat = true;
        }
    }

    if (! infile.is_open() && forced_read) {
        /* Assign some default parameters
         for use in this run. */

        eps_dielect = 15.0;
        sgm_conductivity = 0.005;
        eno_ns_surfref = 301.0;
        frq_mhz = 300.0;
        radio_climate = 5;
        pol = 0;
        conf = 0.50;
        rel = 0.50;
        erp = 0.0;

        /* Write them to a "splat.lrp" file. */

        outfile = fopen("splat.lrp", "w");

        fprintf(outfile,
                "%.3f\t; Earth Dielectric Constant (Relative permittivity)\n",
                eps_dielect);
        fprintf(outfile, "%.3f\t; Earth Conductivity (Siemens per meter)\n",
                sgm_conductivity);
        fprintf(outfile, "%.3f\t; Atmospheric Bending Constant (N-Units)\n",
                eno_ns_surfref);
        fprintf(outfile, "%.3f\t; Frequency in MHz (20 MHz to 20 GHz)\n",
                frq_mhz);
        fprintf(outfile, "%d\t; Radio Climate\n", radio_climate);
        fprintf(outfile, "%d\t; Polarization (0 = Horizontal, 1 = Vertical)\n",
                pol);
        fprintf(outfile, "%.2f\t; Fraction of Situations\n", conf);
        fprintf(outfile, "%.2f\t; Fraction of Time\n", rel);
        fprintf(outfile,
                "%.2f\t; Transmitter Effective Radiated Power in Watts or dBm "
                "(optional)\n",
                erp);
        fprintf(outfile, "\nPlease consult SPLAT! documentation for the "
                         "meaning and use of this data.\n");

        fclose(outfile);

        return_value = 1;

        fprintf(stderr,
                "\n\n%c*** There were problems reading your \"%s\" file! "
                "***\nA \"splat.lrp\" file was written to your directory with "
                "default data.\n",
                7, filename.c_str());
    }

    else if (forced_read == 0)
        return_value = 0;

    if (forced_read && (! ok)) {
        eps_dielect = 15.0;
        sgm_conductivity = 0.005;
        eno_ns_surfref = 301.0;
        frq_mhz = 300.0;
        radio_climate = 5;
        pol = 0;
        conf = 0.50;
        rel = 0.50;
        erp = 0.0;

        fprintf(stderr,
                "Default parameters have been assumed for this analysis.\n");

        return_value = 1;
    }

    return (return_value);
}
