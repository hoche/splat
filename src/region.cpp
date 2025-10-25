/** region.cpp
 *
 * File created by Peter Watkins (KE7IST) 1/8/18.
 * Derived from original project code.
 * Splat!
 * @copyright 1997 - 2018 John A. Magliacane (KD2BD) and contributors.
 * See revision control history for contributions.
 * This file is covered by the LICENSE.md file in the root of this project.
 */

#include "region.h"

#include "site.h"
#include <cstdio>
#include <cstring>
#include <string>

void Region::LoadSignalColors(const Site &xmtr) {
    int x, y, ok, val[4];
    char buffer[80];
    char *pointer = NULL;
    FILE *fd = NULL;

    // Generate .scf filename from xmtr filename
    std::string base = xmtr.filename;
    size_t dot_pos = base.find('.');
    std::string filename;
    if (dot_pos != std::string::npos) {
        filename = base.substr(0, dot_pos) + ".scf";
    } else {
        filename = base + ".scf";
    }

    /* Default values */

    level[0] = 128;
    color[0][0] = 255;
    color[0][1] = 0;
    color[0][2] = 0;

    level[1] = 118;
    color[1][0] = 255;
    color[1][1] = 165;
    color[1][2] = 0;

    level[2] = 108;
    color[2][0] = 255;
    color[2][1] = 206;
    color[2][2] = 0;

    level[3] = 98;
    color[3][0] = 255;
    color[3][1] = 255;
    color[3][2] = 0;

    level[4] = 88;
    color[4][0] = 184;
    color[4][1] = 255;
    color[4][2] = 0;

    level[5] = 78;
    color[5][0] = 0;
    color[5][1] = 255;
    color[5][2] = 0;

    level[6] = 68;
    color[6][0] = 0;
    color[6][1] = 208;
    color[6][2] = 0;

    level[7] = 58;
    color[7][0] = 0;
    color[7][1] = 196;
    color[7][2] = 196;

    level[8] = 48;
    color[8][0] = 0;
    color[8][1] = 148;
    color[8][2] = 255;

    level[9] = 38;
    color[9][0] = 80;
    color[9][1] = 80;
    color[9][2] = 255;

    level[10] = 28;
    color[10][0] = 0;
    color[10][1] = 38;
    color[10][2] = 255;

    level[11] = 18;
    color[11][0] = 142;
    color[11][1] = 63;
    color[11][2] = 255;

    level[12] = 8;
    color[12][0] = 140;
    color[12][1] = 0;
    color[12][2] = 128;

    levels = 13;

    fd = fopen("splat.scf", "r");

    if (fd == NULL)
        fd = fopen(filename.c_str(), "r");

    if (fd == NULL) {
        fd = fopen(filename.c_str(), "w");

        fprintf(
            fd,
            "; SPLAT! Auto-generated Signal Color Definition (\"%s\") File\n",
            filename.c_str());
        fprintf(fd, ";\n; Format for the parameters held in this file is as "
                    "follows:\n;\n");
        fprintf(fd, ";    dBuV/m: red, green, blue\n;\n");
        fprintf(
            fd,
            "; ...where \"dBuV/m\" is the signal strength (in dBuV/m) and\n");
        fprintf(fd, "; \"red\", \"green\", and \"blue\" are the corresponding "
                    "RGB color\n");
        fprintf(
            fd,
            "; definitions ranging from 0 to 255 for the region specified.\n");
        fprintf(
            fd,
            ";\n; The following parameters may be edited and/or expanded\n");
        fprintf(fd,
                "; for future runs of SPLAT!  A total of 32 contour regions\n");
        fprintf(fd, "; may be defined in this file.\n;\n;\n");

        for (x = 0; x < levels; x++)
            fprintf(fd, "%3d: %3d, %3d, %3d\n", level[x], color[x][0],
                    color[x][1], color[x][2]);

        fclose(fd);
    }

    else {
        x = 0;
        fgets(buffer, 80, fd);

        while (x < 32 && feof(fd) == 0) {
            pointer = strchr(buffer, ';');

            if (pointer != NULL)
                *pointer = 0;

            ok = sscanf(buffer, "%d: %d, %d, %d", &val[0], &val[1], &val[2],
                        &val[3]);

            if (ok == 4) {
                for (y = 0; y < 4; y++) {
                    if (val[y] > 255)
                        val[y] = 255;

                    if (val[y] < 0)
                        val[y] = 0;
                }

                level[x] = val[0];
                color[x][0] = val[1];
                color[x][1] = val[2];
                color[x][2] = val[3];
                x++;
            }

            fgets(buffer, 80, fd);
        }

        fclose(fd);
        levels = x;
    }
}

void Region::LoadDBMColors(const Site &xmtr) {
    int x, y, ok, val[4];
    char buffer[80];
    char *pointer = NULL;
    FILE *fd = NULL;

    // Generate .dcf filename from xmtr filename
    std::string base = xmtr.filename;
    size_t dot_pos = base.find('.');
    std::string filename;
    if (dot_pos != std::string::npos) {
        filename = base.substr(0, dot_pos) + ".dcf";
    } else {
        filename = base + ".dcf";
    }

    /* Default values */

    level[0] = 0;
    color[0][0] = 255;
    color[0][1] = 0;
    color[0][2] = 0;

    level[1] = -10;
    color[1][0] = 255;
    color[1][1] = 128;
    color[1][2] = 0;

    level[2] = -20;
    color[2][0] = 255;
    color[2][1] = 165;
    color[2][2] = 0;

    level[3] = -30;
    color[3][0] = 255;
    color[3][1] = 206;
    color[3][2] = 0;

    level[4] = -40;
    color[4][0] = 255;
    color[4][1] = 255;
    color[4][2] = 0;

    level[5] = -50;
    color[5][0] = 184;
    color[5][1] = 255;
    color[5][2] = 0;

    level[6] = -60;
    color[6][0] = 0;
    color[6][1] = 255;
    color[6][2] = 0;

    level[7] = -70;
    color[7][0] = 0;
    color[7][1] = 208;
    color[7][2] = 0;

    level[8] = -80;
    color[8][0] = 0;
    color[8][1] = 196;
    color[8][2] = 196;

    level[9] = -90;
    color[9][0] = 0;
    color[9][1] = 148;
    color[9][2] = 255;

    level[10] = -100;
    color[10][0] = 80;
    color[10][1] = 80;
    color[10][2] = 255;

    level[11] = -110;
    color[11][0] = 0;
    color[11][1] = 38;
    color[11][2] = 255;

    level[12] = -120;
    color[12][0] = 142;
    color[12][1] = 63;
    color[12][2] = 255;

    level[13] = -130;
    color[13][0] = 196;
    color[13][1] = 54;
    color[13][2] = 255;

    level[14] = -140;
    color[14][0] = 255;
    color[14][1] = 0;
    color[14][2] = 255;

    level[15] = -150;
    color[15][0] = 255;
    color[15][1] = 194;
    color[15][2] = 204;

    levels = 16;

    fd = fopen("splat.dcf", "r");

    if (fd == NULL)
        fd = fopen(filename.c_str(), "r");

    if (fd == NULL) {
        fd = fopen(filename.c_str(), "w");

        fprintf(fd,
                "; SPLAT! Auto-generated DBM Signal Level Color Definition "
                "(\"%s\") File\n",
                filename.c_str());
        fprintf(fd, ";\n; Format for the parameters held in this file is as "
                    "follows:\n;\n");
        fprintf(fd, ";    dBm: red, green, blue\n;\n");
        fprintf(fd, "; ...where \"dBm\" is the received signal power level "
                    "between +40 dBm\n");
        fprintf(fd, "; and -200 dBm, and \"red\", \"green\", and \"blue\" are "
                    "the corresponding\n");
        fprintf(fd, "; RGB color definitions ranging from 0 to 255 for the "
                    "region specified.\n");
        fprintf(
            fd,
            ";\n; The following parameters may be edited and/or expanded\n");
        fprintf(fd,
                "; for future runs of SPLAT!  A total of 32 contour regions\n");
        fprintf(fd, "; may be defined in this file.\n;\n;\n");

        for (x = 0; x < levels; x++)
            fprintf(fd, "%+4d: %3d, %3d, %3d\n", level[x], color[x][0],
                    color[x][1], color[x][2]);

        fclose(fd);
    }

    else {
        x = 0;
        fgets(buffer, 80, fd);

        while (x < 32 && feof(fd) == 0) {
            pointer = strchr(buffer, ';');

            if (pointer != NULL)
                *pointer = 0;

            ok = sscanf(buffer, "%d: %d, %d, %d", &val[0], &val[1], &val[2],
                        &val[3]);

            if (ok == 4) {
                if (val[0] < -200)
                    val[0] = -200;

                if (val[0] > +40)
                    val[0] = +40;

                level[x] = val[0];

                for (y = 1; y < 4; y++) {
                    if (val[y] > 255)
                        val[y] = 255;

                    if (val[y] < 0)
                        val[y] = 0;
                }

                color[x][0] = val[1];
                color[x][1] = val[2];
                color[x][2] = val[3];
                x++;
            }

            fgets(buffer, 80, fd);
        }

        fclose(fd);
        levels = x;
    }
}

void Region::LoadLossColors(const Site &xmtr) {
    int x, y, ok, val[4];
    char buffer[80];
    char *pointer = NULL;
    FILE *fd = NULL;

    // Generate .lcf filename from xmtr filename
    std::string base = xmtr.filename;
    size_t dot_pos = base.find('.');
    std::string filename;
    if (dot_pos != std::string::npos) {
        filename = base.substr(0, dot_pos) + ".lcf";
    } else {
        filename = base + ".lcf";
    }

    /* Default values */

    level[0] = 80;
    color[0][0] = 255;
    color[0][1] = 0;
    color[0][2] = 0;

    level[1] = 90;
    color[1][0] = 255;
    color[1][1] = 128;
    color[1][2] = 0;

    level[2] = 100;
    color[2][0] = 255;
    color[2][1] = 165;
    color[2][2] = 0;

    level[3] = 110;
    color[3][0] = 255;
    color[3][1] = 206;
    color[3][2] = 0;

    level[4] = 120;
    color[4][0] = 255;
    color[4][1] = 255;
    color[4][2] = 0;

    level[5] = 130;
    color[5][0] = 184;
    color[5][1] = 255;
    color[5][2] = 0;

    level[6] = 140;
    color[6][0] = 0;
    color[6][1] = 255;
    color[6][2] = 0;

    level[7] = 150;
    color[7][0] = 0;
    color[7][1] = 208;
    color[7][2] = 0;

    level[8] = 160;
    color[8][0] = 0;
    color[8][1] = 196;
    color[8][2] = 196;

    level[9] = 170;
    color[9][0] = 0;
    color[9][1] = 148;
    color[9][2] = 255;

    level[10] = 180;
    color[10][0] = 80;
    color[10][1] = 80;
    color[10][2] = 255;

    level[11] = 190;
    color[11][0] = 0;
    color[11][1] = 38;
    color[11][2] = 255;

    level[12] = 200;
    color[12][0] = 142;
    color[12][1] = 63;
    color[12][2] = 255;

    level[13] = 210;
    color[13][0] = 196;
    color[13][1] = 54;
    color[13][2] = 255;

    level[14] = 220;
    color[14][0] = 255;
    color[14][1] = 0;
    color[14][2] = 255;

    level[15] = 230;
    color[15][0] = 255;
    color[15][1] = 194;
    color[15][2] = 204;

    levels = 16;

    fd = fopen("splat.lcf", "r");

    if (fd == NULL)
        fd = fopen(filename.c_str(), "r");

    if (fd == NULL) {
        fd = fopen(filename.c_str(), "w");

        fprintf(fd,
                "; SPLAT! Auto-generated Path-Loss Color Definition (\"%s\") "
                "File\n",
                filename.c_str());
        fprintf(fd, ";\n; Format for the parameters held in this file is as "
                    "follows:\n;\n");
        fprintf(fd, ";    dB: red, green, blue\n;\n");
        fprintf(fd, "; ...where \"dB\" is the path loss (in dB) and\n");
        fprintf(fd, "; \"red\", \"green\", and \"blue\" are the corresponding "
                    "RGB color\n");
        fprintf(
            fd,
            "; definitions ranging from 0 to 255 for the region specified.\n");
        fprintf(
            fd,
            ";\n; The following parameters may be edited and/or expanded\n");
        fprintf(fd,
                "; for future runs of SPLAT!  A total of 32 contour regions\n");
        fprintf(fd, "; may be defined in this file.\n;\n;\n");

        for (x = 0; x < levels; x++)
            fprintf(fd, "%3d: %3d, %3d, %3d\n", level[x], color[x][0],
                    color[x][1], color[x][2]);

        fclose(fd);
    }

    else {
        x = 0;
        fgets(buffer, 80, fd);

        while (x < 32 && feof(fd) == 0) {
            pointer = strchr(buffer, ';');

            if (pointer != NULL)
                *pointer = 0;

            ok = sscanf(buffer, "%d: %d, %d, %d", &val[0], &val[1], &val[2],
                        &val[3]);

            if (ok == 4) {
                for (y = 0; y < 4; y++) {
                    if (val[y] > 255)
                        val[y] = 255;

                    if (val[y] < 0)
                        val[y] = 0;
                }

                level[x] = val[0];
                color[x][0] = val[1];
                color[x][1] = val[2];
                color[x][2] = val[3];
                x++;
            }

            fgets(buffer, 80, fd);
        }

        fclose(fd);
        levels = x;
    }
}
