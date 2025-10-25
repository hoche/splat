/** @file report.cpp
 *
 * File created by Peter Watkins (KE7IST) 1/8/18.
 * Derived from original project code.
 * Splat!
 * @copyright 1997 - 2018 John A. Magliacane (KD2BD) and contributors.
 * See revision control history for contributions.
 * This file is covered by the LICENSE.md file in the root of this project.
 */

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unistd.h>
#include <vector>

#include "dem.h"
#include "elevation_map.h"
#include "itwom3.0.h"
#include "lrp.h"
#include "antenna_pattern.h"
#include "path.h"
#include "report.h"
#include "sdf.h"
#include "site.h"
#include "splat_run.h"
#include "utilities.h"


void Report::PathReport(const Site &source, const Site &destination,
                        const std::string &name, bool graph_it, elev_t elev[],
                        const AntennaPattern &pat, const Lrp &lrp) {
    /* This function writes a SPLAT! Path Report (name.txt) to
     the filesystem.  If (graph_it == 1), then gnuplot is invoked
     to generate an appropriate output file indicating the ITM
     model loss between the source and destination locations.
     "filename" is the name assigned to the output file generated
     by gnuplot.  The filename extension is used to set gnuplot's
     terminal setting and output file type.  If no extension is
     found, .png is assumed. */

    int x, y, z, errnum = 0;
    char basename[255], term[30], ext[15], strmode[100], report_name[80],
        block = 0, propstring[20];
    double maxloss = -100000.0, minloss = 100000.0, loss = 0.0, haavt, angle1,
           angle2, azimuth, pattern = 1.0, patterndB = 0.0, total_loss = 0.0,
           cos_xmtr_angle, cos_test_angle = 0.0, source_alt, test_alt, dest_alt,
           source_alt2, dest_alt2, distance, elevation, four_thirds_earth,
           field_strength, free_space_loss = 0.0, eirp = 0.0, voltage, rxp, dBm,
           power_density;
    FILE *fd = NULL, *fd2 = NULL;

    Path path(sr.arraysize, sr.ppd);
    sprintf(report_name, "%s-to-%s.txt", source.name.c_str(),
            destination.name.c_str());

    four_thirds_earth = FOUR_THIRDS * sr.earthradius;

    for (x = 0; report_name[x] != 0; x++)
        if (report_name[x] == 32 || report_name[x] == 17 ||
            report_name[x] == 92 || report_name[x] == 42 ||
            report_name[x] == 47)
            report_name[x] = '_';

    fd2 = fopen(report_name, "w");

    fprintf(fd2, "\n\t\t--==[ %s v%s Path Analysis ]==--\n\n",
            SplatRun::splat_name.c_str(), SplatRun::splat_version.c_str());
    fprintf(fd2, "%s\n\n", dashes.c_str());
    fprintf(fd2, "Transmitter site: %s\n", source.name.c_str());

    if (source.lat >= 0.0) {
        fprintf(fd2, "Site location: %.4f North / %.4f West", source.lat,
                source.lon);
        fprintf(fd2, " (%s N / ", Utilities::dec2dms(source.lat).c_str());
    }

    else {

        fprintf(fd2, "Site location: %.4f South / %.4f West", -source.lat,
                source.lon);
        fprintf(fd2, " (%s S / ", Utilities::dec2dms(source.lat).c_str());
    }

    fprintf(fd2, "%s W)\n", Utilities::dec2dms(source.lon).c_str());

    if (sr.metric) {
        fprintf(fd2, "Ground elevation: %.2f meters AMSL\n",
                METERS_PER_FOOT * em.GetElevation(source));
        fprintf(fd2, "Antenna height: %.2f meters AGL / %.2f meters AMSL\n",
                METERS_PER_FOOT * source.alt,
                METERS_PER_FOOT * (source.alt + em.GetElevation(source)));
    }

    else {
        fprintf(fd2, "Ground elevation: %.2f feet AMSL\n",
                em.GetElevation(source));
        fprintf(fd2, "Antenna height: %.2f feet AGL / %.2f feet AMSL\n",
                source.alt, source.alt + em.GetElevation(source));
    }

    haavt = em.haat(path, source);

    if (haavt > -4999.0) {
        if (sr.metric)
            fprintf(fd2, "Antenna height above average terrain: %.2f meters\n",
                    METERS_PER_FOOT * haavt);
        else
            fprintf(fd2, "Antenna height above average terrain: %.2f feet\n",
                    haavt);
    }

    azimuth = source.Azimuth(destination);
    angle1 = em.ElevationAngle(path, source, destination);
    angle2 = em.ElevationAngle2(path, source, destination, sr.earthradius);

    if (pat.got_azimuth_pattern || pat.got_elevation_pattern) {
        x = (int)rint(10.0 * (10.0 - angle2));

        if (x >= 0 && x <= 1000)
            pattern = (double)pat.antenna_pattern[(int)rint(azimuth)][x];

        patterndB = 20.0 * log10(pattern);
    }

    if (sr.metric)
        fprintf(fd2, "Distance to %s: %.2f kilometers\n",
                destination.name.c_str(),
                KM_PER_MILE * source.Distance(destination));

    else
        fprintf(fd2, "Distance to %s: %.2f miles\n", destination.name.c_str(),
                source.Distance(destination));

    fprintf(fd2, "Azimuth to %s: %.2f degrees\n", destination.name.c_str(),
            azimuth);

    if (angle1 >= 0.0)
        fprintf(fd2, "Elevation angle to %s: %+.4f degrees\n",
                destination.name.c_str(), angle1);

    else
        fprintf(fd2, "Depression angle to %s: %+.4f degrees\n",
                destination.name.c_str(), angle1);

    if ((angle2 - angle1) > 0.0001) {
        if (angle2 < 0.0)
            fprintf(fd2, "Depression");
        else
            fprintf(fd2, "Elevation");

        fprintf(fd2, " angle to the first obstruction: %+.4f degrees\n",
                angle2);
    }

    fprintf(fd2, "\n%s\n\n", dashes.c_str());

    /* Receiver */

    fprintf(fd2, "Receiver site: %s\n", destination.name.c_str());

    if (destination.lat >= 0.0) {
        fprintf(fd2, "Site location: %.4f North / %.4f West", destination.lat,
                destination.lon);
        fprintf(fd2, " (%s N / ", Utilities::dec2dms(destination.lat).c_str());
    }

    else {
        fprintf(fd2, "Site location: %.4f South / %.4f West", -destination.lat,
                destination.lon);
        fprintf(fd2, " (%s S / ", Utilities::dec2dms(destination.lat).c_str());
    }

    fprintf(fd2, "%s W)\n", Utilities::dec2dms(destination.lon).c_str());

    if (sr.metric) {
        fprintf(fd2, "Ground elevation: %.2f meters AMSL\n",
                METERS_PER_FOOT * em.GetElevation(destination));
        fprintf(fd2, "Antenna height: %.2f meters AGL / %.2f meters AMSL\n",
                METERS_PER_FOOT * destination.alt,
                METERS_PER_FOOT *
                    (destination.alt + em.GetElevation(destination)));
    }

    else {
        fprintf(fd2, "Ground elevation: %.2f feet AMSL\n",
                em.GetElevation(destination));
        fprintf(fd2, "Antenna height: %.2f feet AGL / %.2f feet AMSL\n",
                destination.alt,
                destination.alt + em.GetElevation(destination));
    }

    haavt = em.haat(path, destination);

    if (haavt > -4999.0) {
        if (sr.metric)
            fprintf(fd2, "Antenna height above average terrain: %.2f meters\n",
                    METERS_PER_FOOT * haavt);
        else
            fprintf(fd2, "Antenna height above average terrain: %.2f feet\n",
                    haavt);
    }

    if (sr.metric)
        fprintf(fd2, "Distance to %s: %.2f kilometers\n", source.name.c_str(),
                KM_PER_MILE * source.Distance(destination));

    else
        fprintf(fd2, "Distance to %s: %.2f miles\n", source.name.c_str(),
                source.Distance(destination));

    azimuth = destination.Azimuth(source);

    angle1 = em.ElevationAngle(path, destination, source);
    angle2 = em.ElevationAngle2(path, destination, source, sr.earthradius);

    fprintf(fd2, "Azimuth to %s: %.2f degrees\n", source.name.c_str(), azimuth);

    if (angle1 >= 0.0)
        fprintf(fd2, "Elevation angle to %s: %+.4f degrees\n",
                source.name.c_str(), angle1);

    else
        fprintf(fd2, "Depression angle to %s: %+.4f degrees\n",
                source.name.c_str(), angle1);

    if ((angle2 - angle1) > 0.0001) {
        if (angle2 < 0.0)
            fprintf(fd2, "Depression");
        else
            fprintf(fd2, "Elevation");

        fprintf(fd2, " angle to the first obstruction: %+.4f degrees\n",
                angle2);
    }

    fprintf(fd2, "\n%s\n\n", dashes.c_str());

    if (lrp.frq_mhz > 0.0) {
        if (sr.propagation_model == PROP_ITM)
            fprintf(fd2, "Longley-Rice Parameters Used In This Analysis:\n\n");
        else
            fprintf(fd2,
                    "ITWOM Version %.1f Parameters Used In This Analysis:\n\n",
                    ITWOMVersion());

        fprintf(fd2, "Earth's Dielectric Constant: %.3lf\n", lrp.eps_dielect);
        fprintf(fd2, "Earth's Conductivity: %.3lf Siemens/meter\n",
                lrp.sgm_conductivity);
        fprintf(fd2, "Atmospheric Bending Constant (N-units): %.3lf ppm\n",
                lrp.eno_ns_surfref);
        fprintf(fd2, "Frequency: %.3lf MHz\n", lrp.frq_mhz);
        fprintf(fd2, "Radio Climate: %d (", lrp.radio_climate);

        switch (lrp.radio_climate) {
        case 1:
            fprintf(fd2, "Equatorial");
            break;

        case 2:
            fprintf(fd2, "Continental Subtropical");
            break;

        case 3:
            fprintf(fd2, "Maritime Subtropical");
            break;

        case 4:
            fprintf(fd2, "Desert");
            break;

        case 5:
            fprintf(fd2, "Continental Temperate");
            break;

        case 6:
            fprintf(fd2, "Martitime Temperate, Over Land");
            break;

        case 7:
            fprintf(fd2, "Maritime Temperate, Over Sea");
            break;

        default:
            fprintf(fd2, "Unknown");
        }

        fprintf(fd2, ")\nPolarization: %d (", lrp.pol);

        if (lrp.pol == 0)
            fprintf(fd2, "Horizontal");

        if (lrp.pol == 1)
            fprintf(fd2, "Vertical");

        fprintf(fd2, ")\nFraction of Situations: %.1lf%c\n", lrp.conf * 100.0,
                37);
        fprintf(fd2, "Fraction of Time: %.1lf%c\n", lrp.rel * 100.0, 37);

        if (lrp.erp != 0.0) {
            fprintf(fd2, "Transmitter ERP: ");

            if (lrp.erp < 1.0)
                fprintf(fd2, "%.1lf milliwatts", 1000.0 * lrp.erp);

            if (lrp.erp >= 1.0 && lrp.erp < 10.0)
                fprintf(fd2, "%.1lf Watts", lrp.erp);

            if (lrp.erp >= 10.0 && lrp.erp < 10.0e3)
                fprintf(fd2, "%.0lf Watts", lrp.erp);

            if (lrp.erp >= 10.0e3)
                fprintf(fd2, "%.3lf kilowatts", lrp.erp / 1.0e3);

            dBm = 10.0 * (log10(lrp.erp * 1000.0));
            fprintf(fd2, " (%+.2f dBm)\n", dBm);

            /* EIRP = ERP + 2.14 dB */

            fprintf(fd2, "Transmitter EIRP: ");

            eirp = lrp.erp * 1.636816521;

            if (eirp < 1.0)
                fprintf(fd2, "%.1lf milliwatts", 1000.0 * eirp);

            if (eirp >= 1.0 && eirp < 10.0)
                fprintf(fd2, "%.1lf Watts", eirp);

            if (eirp >= 10.0 && eirp < 10.0e3)
                fprintf(fd2, "%.0lf Watts", eirp);

            if (eirp >= 10.0e3)
                fprintf(fd2, "%.3lf kilowatts", eirp / 1.0e3);

            dBm = 10.0 * (log10(eirp * 1000.0));
            fprintf(fd2, " (%+.2f dBm)\n", dBm);
        }

        fprintf(fd2, "\n%s\n\n", dashes.c_str());

        fprintf(fd2, "Summary For The Link Between %s and %s:\n\n",
                source.name.c_str(), destination.name.c_str());

        if (patterndB != 0.0)
            fprintf(fd2, "%s antenna pattern towards %s: %.3f (%.2f dB)\n",
                    source.name.c_str(), destination.name.c_str(), pattern,
                    patterndB);

        path.ReadPath(source, destination, em); /* source=TX, destination=RX */

        /* Copy elevations plus clutter along
         path into the elev[] array. */

        for (x = 1; x < path.length - 1; x++)
            elev[x + 2] =
                METERS_PER_FOOT * (path.elevation[x] == 0.0
                                       ? path.elevation[x]
                                       : (sr.clutter + path.elevation[x]));

        /* Copy ending points without clutter */

        elev[2] = path.elevation[0] * METERS_PER_FOOT;
        elev[path.length + 1] =
            path.elevation[path.length - 1] * METERS_PER_FOOT;

        fd = fopen("profile.gp", "w");

        azimuth = rint(source.Azimuth(destination));

        for (y = 2; y < (path.length - 1);
             y++) /* path.length-1 avoids LR error */
        {
            distance = 5280.0 * path.distance[y];
            source_alt = four_thirds_earth + source.alt + path.elevation[0];
            dest_alt = four_thirds_earth + destination.alt + path.elevation[y];
            dest_alt2 = dest_alt * dest_alt;
            source_alt2 = source_alt * source_alt;

            /* Calculate the cosine of the elevation of
             the receiver as seen by the transmitter. */

            cos_xmtr_angle =
                ((source_alt2) + (distance * distance) - (dest_alt2)) /
                (2.0 * source_alt * distance);

            if (pat.got_elevation_pattern) {
                /* If an antenna elevation pattern is available, the
                 following code determines the elevation angle to
                 the first obstruction along the path. */

                for (x = 2, block = 0; x < y && block == 0; x++) {
                    distance = 5280.0 * (path.distance[y] - path.distance[x]);
                    test_alt = four_thirds_earth + path.elevation[x];

                    /* Calculate the cosine of the elevation
                     angle of the terrain (test point)
                     as seen by the transmitter. */

                    cos_test_angle = ((source_alt2) + (distance * distance) -
                                      (test_alt * test_alt)) /
                                     (2.0 * source_alt * distance);

                    /* Compare these two angles to determine if
                     an obstruction exists.  Since we're comparing
                     the cosines of these angles rather than
                     the angles themselves, the sense of the
                     following "if" statement is reversed from
                     what it would be if the angles themselves
                     were compared. */

                    if (cos_xmtr_angle >= cos_test_angle)
                        block = 1;
                }

                /* At this point, we have the elevation angle
                 to the first obstruction (if it exists). */
            }

            /* Determine path loss for each point along
             the path using ITWOM's point_to_point mode
             starting at x=2 (number_of_points = 1), the
             shortest distance terrain can play a role in
             path loss. */

            elev[0] = y - 1; /* (number of points - 1) */

            /* Distance between elevation samples */

            elev[1] =
                METERS_PER_MILE * (path.distance[y] - path.distance[y - 1]);

            if (sr.propagation_model == PROP_ITM)
                point_to_point_ITM(elev, source.alt * METERS_PER_FOOT,
                                   destination.alt * METERS_PER_FOOT,
                                   lrp.eps_dielect, lrp.sgm_conductivity,
                                   lrp.eno_ns_surfref, lrp.frq_mhz,
                                   lrp.radio_climate, lrp.pol, lrp.conf,
                                   lrp.rel, loss, strmode, errnum);
            else
                point_to_point(elev, source.alt * METERS_PER_FOOT,
                               destination.alt * METERS_PER_FOOT,
                               lrp.eps_dielect, lrp.sgm_conductivity,
                               lrp.eno_ns_surfref, lrp.frq_mhz,
                               lrp.radio_climate, lrp.pol, lrp.conf, lrp.rel,
                               loss, strmode, errnum);

            if (block)
                elevation = ((acos(cos_test_angle)) / DEG2RAD) - 90.0;
            else
                elevation = ((acos(cos_xmtr_angle)) / DEG2RAD) - 90.0;

            /* Integrate the antenna's radiation
             pattern into the overall path loss. */

            x = (int)rint(10.0 * (10.0 - elevation));

            if (x >= 0 && x <= 1000) {
                pattern = (double)pat.antenna_pattern[(int)azimuth][x];

                if (pattern != 0.0)
                    patterndB = 20.0 * log10(pattern);
            }

            else
                patterndB = 0.0;

            total_loss = loss - patterndB;

            if (sr.metric)
                fprintf(fd, "%f\t%f\n", KM_PER_MILE * path.distance[y],
                        total_loss);

            else
                fprintf(fd, "%f\t%f\n", path.distance[y], total_loss);

            if (total_loss > maxloss)
                maxloss = total_loss;

            if (total_loss < minloss)
                minloss = total_loss;
        }

        fclose(fd);

        distance = source.Distance(destination);

        if (distance != 0.0) {
            free_space_loss =
                36.6 + (20.0 * log10(lrp.frq_mhz)) + (20.0 * log10(distance));

            fprintf(fd2, "Free space path loss: %.2f dB\n", free_space_loss);
        }

        if (sr.propagation_model == PROP_ITM)
            fprintf(fd2, "Longley-Rice path loss: %.2f dB\n", loss);
        else
            fprintf(fd2, "ITWOM Version %.1f path loss: %.2f dB\n",
                    ITWOMVersion(), loss);

        if (free_space_loss != 0.0)
            fprintf(fd2, "Attenuation due to terrain shielding: %.2f dB\n",
                    loss - free_space_loss);

        if (patterndB != 0.0)
            fprintf(fd2,
                    "Total path loss including %s antenna pattern: %.2f dB\n",
                    source.name.c_str(), total_loss);

        if (lrp.erp != 0.0) {
            field_strength =
                (139.4 + (20.0 * log10(lrp.frq_mhz)) - total_loss) +
                (10.0 * log10(lrp.erp / 1000.0));

            /* dBm is referenced to EIRP */

            rxp = eirp / (pow(10.0, (total_loss / 10.0)));
            dBm = 10.0 * (log10(rxp * 1000.0));
            power_density =
                (eirp / (pow(10.0, (total_loss - free_space_loss) / 10.0)));
            /* divide by 4*PI*distance_in_meters squared */
            power_density /= (4.0 * PI * distance * distance * 2589988.11);

            fprintf(fd2, "Field strength at %s: %.2f dBuV/meter\n",
                    destination.name.c_str(), field_strength);
            fprintf(fd2, "Signal power level at %s: %+.2f dBm\n",
                    destination.name.c_str(), dBm);
            fprintf(fd2,
                    "Signal power density at %s: %+.2f dBW per square meter\n",
                    destination.name.c_str(), 10.0 * log10(power_density));
            voltage =
                1.0e6 *
                sqrt(50.0 * (eirp / (pow(10.0, (total_loss - 2.14) / 10.0))));
            fprintf(
                fd2,
                "Voltage across a 50 ohm dipole at %s: %.2f uV (%.2f dBuV)\n",
                destination.name.c_str(), voltage, 20.0 * log10(voltage));

            voltage =
                1.0e6 *
                sqrt(75.0 * (eirp / (pow(10.0, (total_loss - 2.14) / 10.0))));
            fprintf(
                fd2,
                "Voltage across a 75 ohm dipole at %s: %.2f uV (%.2f dBuV)\n",
                destination.name.c_str(), voltage, 20.0 * log10(voltage));
        }

        fprintf(fd2, "Mode of propagation: ");

		if (sr.propagation_model == PROP_ITM) {
            fprintf(fd2, "%s\n", strmode);
            fprintf(fd2, "Longley-Rice model error number: %d", errnum);
        }

        else {
            if (strcmp(strmode, "L-o-S") == 0)
                fprintf(fd2, "Line of Sight\n");

            if (strncmp(strmode, "1_Hrzn", 6) == 0)
                fprintf(fd2, "Single Horizon ");

            if (strncmp(strmode, "2_Hrzn", 6) == 0)
                fprintf(fd2, "Double Horizon ");

            size_t y = strlen(strmode);

            if (y > 19)
                y = 19;

            for (x = 6; x < (int)y; x++)
                propstring[x - 6] = strmode[x];

            propstring[x] = 0;

            if (strncmp(propstring, "_Diff", 5) == 0)
                fprintf(fd2, "Diffraction Dominant\n");

            if (strncmp(propstring, "_Tropo", 6) == 0)
                fprintf(fd2, "Troposcatter Dominant\n");

            if (strncmp(propstring, "_Peak", 5) == 0)
                fprintf(fd2, "RX at Peak Terrain Along Path\n");

            fprintf(fd2, "ITWOM error number: %d", errnum);
        }

        switch (errnum) {
        case 0:
            fprintf(fd2, " (No error)\n");
            break;

        case 1:
            fprintf(fd2,
                    "\n  Warning: Some parameters are nearly out of range.\n");
            fprintf(fd2, "  Results should be used with caution.\n");
            break;

        case 2:
            fprintf(fd2, "\n  Note: Default parameters have been substituted "
                         "for impossible ones.\n");
            break;

        case 3:
            fprintf(
                fd2,
                "\n  Warning: A combination of parameters is out of range.\n");
            fprintf(fd2, "  Results are probably invalid.\n");
            break;

        default:
            fprintf(fd2, "\n  Warning: Some parameters are out of range.\n");
            fprintf(fd2, "  Results are probably invalid.\n");
        }

        fprintf(fd2, "\n%s\n\n", dashes.c_str());
    }

    fprintf(stdout, "\nPath Loss Report written to: \"%s\"\n", report_name);
    fflush(stdout);

    ObstructionAnalysis(source, destination, lrp.frq_mhz, fd2);

    fclose(fd2);

    /* Skip plotting the graph if ONLY a path-loss report is needed. */

    if (graph_it) {
        if (name[0] == '.') {
            /* Default filename and output file type */

            strncpy(basename, "profile\0", 8);
            strncpy(term, "png\0", 4);
            strncpy(ext, "png\0", 4);
        }

        else {
            /* Extract extension and terminal type from "name" */

            ext[0] = 0;
            size_t y = strlen(name.c_str());
            strncpy(basename, name.c_str(), 254);

            for (x = (int)y - 1; x > 0 && name[x] != '.'; x--)
                ;

            if (x > 0) /* Extension found */
            {
                for (z = x + 1; z <= (int)y && (z - (x + 1)) < 10; z++) {
                    ext[z - (x + 1)] = tolower(name[z]);
                    term[z - (x + 1)] = name[z];
                }

                ext[z - (x + 1)] = 0; /* Ensure an ending 0 */
                term[z - (x + 1)] = 0;
                basename[x] = 0;
            }
        }

        if (ext[0] == 0) /* No extension -- Default is png */
        {
            strncpy(term, "png\0", 4);
            strncpy(ext, "png\0", 4);
        }

        /* Either .ps or .postscript may be used
         as an extension for postscript output. */

        if (strncmp(term, "postscript", 10) == 0)
            strncpy(ext, "ps\0", 3);

        else if (strncmp(ext, "ps", 2) == 0)
            strncpy(term, "postscript enhanced color\0", 26);

        fd = fopen("splat.gp", "w");

        fprintf(fd, "set grid\n");
        fprintf(fd, "set yrange [%2.3f to %2.3f]\n", minloss, maxloss);
        fprintf(fd, "set encoding iso_8859_1\n");
        fprintf(fd, "set term %s\n", term);
        fprintf(fd,
                "set title \"%s Loss Profile Along Path Between %s and %s "
                "(%.2f%c azimuth)\"\n",
                SplatRun::splat_name.c_str(), destination.name.c_str(),
                source.name.c_str(), destination.Azimuth(source), 176);

        if (sr.metric)
            fprintf(
                fd,
                "set xlabel \"Distance Between %s and %s (%.2f kilometers)\"\n",
                destination.name.c_str(), source.name.c_str(),
                KM_PER_MILE * destination.Distance(source));
        else
            fprintf(fd,
                    "set xlabel \"Distance Between %s and %s (%.2f miles)\"\n",
                    destination.name.c_str(), source.name.c_str(),
                    destination.Distance(source));

        if (pat.got_azimuth_pattern || pat.got_elevation_pattern)
            fprintf(fd, "set ylabel \"Total Path Loss (including TX antenna "
                        "pattern) (dB)");
        else {
            if (sr.propagation_model == PROP_ITM)
                fprintf(fd, "set ylabel \"Longley-Rice Path Loss (dB)");
            else
                fprintf(fd, "set ylabel \"ITWOM Version %.1f Path Loss (dB)",
                        ITWOMVersion());
        }

        fprintf(fd, "\"\nset output \"%s.%s\"\n", basename, ext);
        fprintf(fd, "plot \"profile.gp\" title \"Path Loss\" with lines\n");

        fclose(fd);

        x = system("gnuplot splat.gp");

        if (x != -1) {
            if (!sr.gpsav) {
                unlink("splat.gp");
                unlink("profile.gp");
                unlink("reference.gp");
            }

            fprintf(stdout, "Path loss plot written to: \"%s.%s\"\n", basename,
                    ext);
            fflush(stdout);
        }

        else
            fprintf(stderr, "\n*** ERROR: Error occurred invoking gnuplot!\n");
    }

    if (x != -1 && !sr.gpsav)
        unlink("profile.gp");
}

void Report::SiteReport(const Site &xmtr) {
    char report_name[80];
    double terrain;
    int x, azi;
    FILE *fd;

    sprintf(report_name, "%s-site_report.txt", xmtr.name.c_str());

    for (x = 0; report_name[x] != 0; x++)
        if (report_name[x] == 32 || report_name[x] == 17 ||
            report_name[x] == 92 || report_name[x] == 42 ||
            report_name[x] == 47)
            report_name[x] = '_';

    fd = fopen(report_name, "w");

    fprintf(fd, "\n\t--==[ %s v%s Site Analysis Report For: %s ]==--\n\n",
            SplatRun::splat_name.c_str(), SplatRun::splat_version.c_str(),
            xmtr.name.c_str());

    fprintf(fd, "%s\n\n", dashes.c_str());

    if (xmtr.lat >= 0.0) {
        fprintf(fd, "Site location: %.4f North / %.4f West", xmtr.lat,
                xmtr.lon);
        fprintf(fd, " (%s N / ", Utilities::dec2dms(xmtr.lat).c_str());
    }

    else {
        fprintf(fd, "Site location: %.4f South / %.4f West", -xmtr.lat,
                xmtr.lon);
        fprintf(fd, " (%s S / ", Utilities::dec2dms(xmtr.lat).c_str());
    }

    fprintf(fd, "%s W)\n", Utilities::dec2dms(xmtr.lon).c_str());

    if (sr.metric) {
        fprintf(fd, "Ground elevation: %.2f meters AMSL\n",
                METERS_PER_FOOT * em.GetElevation(xmtr));
        fprintf(fd, "Antenna height: %.2f meters AGL / %.2f meters AMSL\n",
                METERS_PER_FOOT * xmtr.alt,
                METERS_PER_FOOT * (xmtr.alt + em.GetElevation(xmtr)));
    }

    else {
        fprintf(fd, "Ground elevation: %.2f feet AMSL\n",
                em.GetElevation(xmtr));
        fprintf(fd, "Antenna height: %.2f feet AGL / %.2f feet AMSL\n",
                xmtr.alt, xmtr.alt + em.GetElevation(xmtr));
    }

    terrain = em.haat(path, xmtr);

    if (terrain > -4999.0) {
        if (sr.metric)
            fprintf(fd, "Antenna height above average terrain: %.2f meters\n\n",
                    METERS_PER_FOOT * terrain);
        else
            fprintf(fd, "Antenna height above average terrain: %.2f feet\n\n",
                    terrain);

        /* Display the average terrain between 2 and 10 miles
         from the transmitter site at azimuths of 0, 45, 90,
         135, 180, 225, 270, and 315 degrees. */

        for (azi = 0; azi <= 315; azi += 45) {
            fprintf(fd, "Average terrain at %3d degrees azimuth: ", azi);
            terrain = em.AverageTerrain(path, xmtr, (double)azi, 2.0, 10.0);

            if (terrain > -4999.0) {
                if (sr.metric)
                    fprintf(fd, "%.2f meters AMSL\n",
                            METERS_PER_FOOT * terrain);
                else
                    fprintf(fd, "%.2f feet AMSL\n", terrain);
            }

            else
                fprintf(fd, "No terrain\n");
        }
    }

    fprintf(fd, "\n%s\n\n", dashes.c_str());
    fclose(fd);
    fprintf(stdout, "\nSite analysis report written to: \"%s\"\n", report_name);
}

void Report::ObstructionAnalysis(const Site &xmtr, const Site &rcvr, double f,
                                 FILE *outfile) {
    /* Perform an obstruction analysis along the
     path between receiver and transmitter. */

    int x;
    Site site_x;
    double h_r, h_t, h_x, h_r_orig, cos_tx_angle, cos_test_angle,
        cos_tx_angle_f1, cos_tx_angle_fpt6, d_tx, d_x, h_r_f1, h_r_fpt6, h_f,
        h_los, lambda = 0.0;
    char string[255], string_fpt6[255], string_f1[255];

    Path path(sr.arraysize, sr.ppd);

    path.ReadPath(xmtr, rcvr, em);
    h_r = em.GetElevation(rcvr) + rcvr.alt + sr.earthradius;
    h_r_f1 = h_r;
    h_r_fpt6 = h_r;
    h_r_orig = h_r;
    h_t = em.GetElevation(xmtr) + xmtr.alt + sr.earthradius;
    d_tx = 5280.0 * rcvr.Distance(xmtr);
    cos_tx_angle =
        ((h_r * h_r) + (d_tx * d_tx) - (h_t * h_t)) / (2.0 * h_r * d_tx);
    cos_tx_angle_f1 = cos_tx_angle;
    cos_tx_angle_fpt6 = cos_tx_angle;

    if (f)
        lambda = 9.8425e8 / (f * 1e6);

    if (sr.clutter > 0.0) {
        fprintf(outfile, "Terrain has been raised by");

        if (sr.metric)
            fprintf(outfile, " %.2f meters", METERS_PER_FOOT * sr.clutter);
        else
            fprintf(outfile, " %.2f feet", sr.clutter);

        fprintf(outfile, " to account for ground sr.clutter.\n\n");
    }

    /* At each point along the path calculate the cosine
     of a sort of "inverse elevation angle" at the receiver.
     From the antenna, 0 deg. looks at the ground, and 90 deg.
     is parallel to the ground.

     Start at the receiver.  If this is the lowest antenna,
     then terrain obstructions will be nearest to it.  (Plus,
     that's the way SPLAT!'s original los() did it.)

     Calculate cosines only.  That's sufficient to compare
     angles and it saves the extra computational burden of
     acos().  However, note the inverted comparison: if
     acos(A) > acos(B), then B > A. */

    for (x = path.length - 1; x > 0; x--) {
        site_x.lat = path.lat[x];
        site_x.lon = path.lon[x];
        site_x.alt = 0.0;

        h_x = em.GetElevation(site_x) + sr.earthradius + sr.clutter;
        d_x = 5280.0 * rcvr.Distance(site_x);

        /* Deal with the LOS path first. */

        cos_test_angle =
            ((h_r * h_r) + (d_x * d_x) - (h_x * h_x)) / (2.0 * h_r * d_x);

        if (cos_tx_angle > cos_test_angle) {
            if (h_r == h_r_orig)
                fprintf(outfile,
                        "Between %s and %s, %s detected obstructions at:\n\n",
                        rcvr.name.c_str(), xmtr.name.c_str(),
                        SplatRun::splat_name.c_str());

            if (site_x.lat >= 0.0) {
                if (sr.metric)
                    fprintf(outfile,
                            "   %8.4f N,%9.4f W, %5.2f kilometers, %6.2f "
                            "meters AMSL\n",
                            site_x.lat, site_x.lon,
                            KM_PER_MILE * (d_x / 5280.0),
                            METERS_PER_FOOT * (h_x - sr.earthradius));
                else
                    fprintf(
                        outfile,
                        "   %8.4f N,%9.4f W, %5.2f miles, %6.2f feet AMSL\n",
                        site_x.lat, site_x.lon, d_x / 5280.0,
                        h_x - sr.earthradius);
            }

            else {
                if (sr.metric)
                    fprintf(outfile,
                            "   %8.4f S,%9.4f W, %5.2f kilometers, %6.2f "
                            "meters AMSL\n",
                            -site_x.lat, site_x.lon,
                            KM_PER_MILE * (d_x / 5280.0),
                            METERS_PER_FOOT * (h_x - sr.earthradius));
                else

                    fprintf(
                        outfile,
                        "   %8.4f S,%9.4f W, %5.2f miles, %6.2f feet AMSL\n",
                        -site_x.lat, site_x.lon, d_x / 5280.0,
                        h_x - sr.earthradius);
            }
        }

        while (cos_tx_angle > cos_test_angle) {
            h_r += 1;
            cos_test_angle =
                ((h_r * h_r) + (d_x * d_x) - (h_x * h_x)) / (2.0 * h_r * d_x);
            cos_tx_angle = ((h_r * h_r) + (d_tx * d_tx) - (h_t * h_t)) /
                           (2.0 * h_r * d_tx);
        }

        if (f) {
            /* Now clear the first Fresnel zone... */

            cos_tx_angle_f1 =
                ((h_r_f1 * h_r_f1) + (d_tx * d_tx) - (h_t * h_t)) /
                (2.0 * h_r_f1 * d_tx);
            h_los = sqrt(h_r_f1 * h_r_f1 + d_x * d_x -
                         2 * h_r_f1 * d_x * cos_tx_angle_f1);
            h_f = h_los - sqrt(lambda * d_x * (d_tx - d_x) / d_tx);

            while (h_f < h_x) {
                h_r_f1 += 1;
                cos_tx_angle_f1 =
                    ((h_r_f1 * h_r_f1) + (d_tx * d_tx) - (h_t * h_t)) /
                    (2.0 * h_r_f1 * d_tx);
                h_los = sqrt(h_r_f1 * h_r_f1 + d_x * d_x -
                             2 * h_r_f1 * d_x * cos_tx_angle_f1);
                h_f = h_los - sqrt(lambda * d_x * (d_tx - d_x) / d_tx);
            }

            /* and clear the 60% F1 zone. */

            cos_tx_angle_fpt6 =
                ((h_r_fpt6 * h_r_fpt6) + (d_tx * d_tx) - (h_t * h_t)) /
                (2.0 * h_r_fpt6 * d_tx);
            h_los = sqrt(h_r_fpt6 * h_r_fpt6 + d_x * d_x -
                         2 * h_r_fpt6 * d_x * cos_tx_angle_fpt6);
            h_f = h_los -
                  sr.fzone_clearance * sqrt(lambda * d_x * (d_tx - d_x) / d_tx);

            while (h_f < h_x) {
                h_r_fpt6 += 1;
                cos_tx_angle_fpt6 =
                    ((h_r_fpt6 * h_r_fpt6) + (d_tx * d_tx) - (h_t * h_t)) /
                    (2.0 * h_r_fpt6 * d_tx);
                h_los = sqrt(h_r_fpt6 * h_r_fpt6 + d_x * d_x -
                             2 * h_r_fpt6 * d_x * cos_tx_angle_fpt6);
                h_f = h_los - sr.fzone_clearance *
                                  sqrt(lambda * d_x * (d_tx - d_x) / d_tx);
            }
        }
    }

    if (h_r > h_r_orig) {
        if (sr.metric)
            snprintf(string, 150,
                     "\nAntenna at %s must be raised to at least %.2f meters "
                     "AGL\nto clear all obstructions detected by %s.\n",
                     rcvr.name.c_str(),
                     METERS_PER_FOOT *
                         (h_r - em.GetElevation(rcvr) - sr.earthradius),
                     SplatRun::splat_name.c_str());
        else
            snprintf(string, 150,
                     "\nAntenna at %s must be raised to at least %.2f feet "
                     "AGL\nto clear all obstructions detected by %s.\n",
                     rcvr.name.c_str(),
                     h_r - em.GetElevation(rcvr) - sr.earthradius,
                     SplatRun::splat_name.c_str());
    }

    else
        snprintf(string, 150,
                 "\nNo obstructions to LOS path due to terrain were detected "
                 "by %s\n",
                 SplatRun::splat_name.c_str());

    if (f) {
        if (h_r_fpt6 > h_r_orig) {
            if (sr.metric)
                snprintf(
                    string_fpt6, 150,
                    "\nAntenna at %s must be raised to at least %.2f meters "
                    "AGL\nto clear %.0f%c of the first Fresnel zone.\n",
                    rcvr.name.c_str(),
                    METERS_PER_FOOT *
                        (h_r_fpt6 - em.GetElevation(rcvr) - sr.earthradius),
                    sr.fzone_clearance * 100.0, 37);

            else
                snprintf(string_fpt6, 150,
                         "\nAntenna at %s must be raised to at least %.2f feet "
                         "AGL\nto clear %.0f%c of the first Fresnel zone.\n",
                         rcvr.name.c_str(),
                         h_r_fpt6 - em.GetElevation(rcvr) - sr.earthradius,
                         sr.fzone_clearance * 100.0, 37);
        }

        else
            snprintf(string_fpt6, 150,
                     "\n%.0f%c of the first Fresnel zone is clear.\n",
                     sr.fzone_clearance * 100.0, 37);

        if (h_r_f1 > h_r_orig) {
            if (sr.metric)
                snprintf(string_f1, 150,
                         "\nAntenna at %s must be raised to at least %.2f "
                         "meters AGL\nto clear the first Fresnel zone.\n",
                         rcvr.name.c_str(),
                         METERS_PER_FOOT *
                             (h_r_f1 - em.GetElevation(rcvr) - sr.earthradius));

            else
                snprintf(string_f1, 150,
                         "\nAntenna at %s must be raised to at least %.2f feet "
                         "AGL\nto clear the first Fresnel zone.\n",
                         rcvr.name.c_str(),
                         h_r_f1 - em.GetElevation(rcvr) - sr.earthradius);

        }

        else
            snprintf(string_f1, 150, "\nThe first Fresnel zone is clear.\n");
    }

    fprintf(outfile, "%s", string);

    if (f) {
        fprintf(outfile, "%s", string_f1);
        fprintf(outfile, "%s", string_fpt6);
    }
}
