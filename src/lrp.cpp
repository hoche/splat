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
#include <iostream>
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
    char filename[255], string[80], *pointer = NULL, return_value = 0;
    int iin = 0, ok = 0, x;
    FILE *fd = NULL, *outfile = NULL;

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

    for (x = 0; txsite.filename[x] != '.' && txsite.filename[x] != 0 && x < 250;
         x++)
        filename[x] = txsite.filename[x];

    filename[x] = '.';
    filename[x + 1] = 'l';
    filename[x + 2] = 'r';
    filename[x + 3] = 'p';
    filename[x + 4] = 0;

    fd = fopen(filename, "r");

    if (fd == NULL) {
        /* Load default "splat.lrp" file */

        strcpy(filename, "splat.lrp");
        fd = fopen(filename, "r");
    }

    if (fd != NULL) {
        fgets(string, 80, fd);

        pointer = strchr(string, ';');

        if (pointer != NULL)
            *pointer = 0;

        ok = sscanf(string, "%lf", &din);

        if (ok) {
            eps_dielect = din;

            fgets(string, 80, fd);

            pointer = strchr(string, ';');

            if (pointer != NULL)
                *pointer = 0;

            ok = sscanf(string, "%lf", &din);
        }

        if (ok) {
            sgm_conductivity = din;

            fgets(string, 80, fd);

            pointer = strchr(string, ';');

            if (pointer != NULL)
                *pointer = 0;

            ok = sscanf(string, "%lf", &din);
        }

        if (ok) {
            eno_ns_surfref = din;

            fgets(string, 80, fd);

            pointer = strchr(string, ';');

            if (pointer != NULL)
                *pointer = 0;

            ok = sscanf(string, "%lf", &din);
        }

        if (ok) {
            frq_mhz = din;

            fgets(string, 80, fd);

            pointer = strchr(string, ';');

            if (pointer != NULL)
                *pointer = 0;

            ok = sscanf(string, "%d", &iin);
        }

        if (ok) {
            radio_climate = iin;

            fgets(string, 80, fd);

            pointer = strchr(string, ';');

            if (pointer != NULL)
                *pointer = 0;

            ok = sscanf(string, "%d", &iin);
        }

        if (ok) {
            pol = iin;

            fgets(string, 80, fd);

            pointer = strchr(string, ';');

            if (pointer != NULL)
                *pointer = 0;

            ok = sscanf(string, "%lf", &din);
        }

        if (ok) {
            conf = din;

            fgets(string, 80, fd);

            pointer = strchr(string, ';');

            if (pointer != NULL)
                *pointer = 0;

            ok = sscanf(string, "%lf", &din);
        }

        if (ok) {
            rel = din;
            din = 0.0;
            return_value = 1;

            if (fgets(string, 80, fd) != NULL) {
                pointer = strchr(string, ';');

                if (pointer != NULL)
                    *pointer = 0;

                if (sscanf(string, "%lf", &din))
                    erp = din;

                /* ERP in SPLAT! is referenced to 1 Watt
                 into a dipole (0 dBd).  If ERP is
                 expressed in dBm (referenced to a
                 0 dBi radiator), convert dBm in EIRP
                 to ERP.  */

                if ((strstr(string, "dBm") != NULL) ||
                    (strstr(string, "dbm") != NULL))
                    erp = (pow(10.0, (erp - 32.14) / 10.0));
            }
        }

        fclose(fd);

        if (forced_erp != -1.0)
            erp = forced_erp;

        if (forced_freq >= 20.0 && forced_freq <= 20000.0)
            frq_mhz = forced_freq;

        if (ok) {
            filenameOut = filename;
            loadPat = true;
        }
    }

    if (fd == NULL && forced_read) {
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
                7, filename);
    }

    else if (forced_read == 0)
        return_value = 0;

    if (forced_read && (fd == NULL || ok == 0)) {
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
