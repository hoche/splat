/** @file sdf_bz.cpp
 *
 * File created by Peter Watkins (KE7IST) 4/31/20.
 * Derived from original project code.
 * Splat!
 * @copyright 1997 - 2018 John A. Magliacane (KD2BD) and contributors.
 * See revision control history for contributions.
 * This file is covered by the LICENSE.md file in the root of this project.
 */

#include "sdf_bz.h"
#include "dem.h"
#include "elevation_map.h"
#include "lrp.h"
#include "antenna_pattern.h"
#include "path.h"
#include "sdf.h"
#include "site.h"
#include "splat_run.h"
#include <bzlib.h>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#define BZBUFFER 65536

SdfBz::SdfBz(const std::string &path, const SplatRun &sr) : Sdf(path, sr) {
    suffix = ".sdf.bz2";
}

char *SdfBz::BZfgets(BZFILE *bzfd, unsigned length) {
    /* This function returns at most one less than 'length' number
     of characters from a bz2 compressed file whose file descriptor
     is pointed to by *bzfd.  In operation, a buffer is filled with
     uncompressed data (size = BZBUFFER), which is then parsed
     and doled out as NULL terminated character strings every time
     this function is invoked.  A NULL string indicates an EOF
     or error condition. */

    static int x, y, nBuf;
    static char buffer[BZBUFFER + 1], output[BZBUFFER + 1];
    bool done = false, opened = false;

    if (opened != 1 && bzerror == BZ_OK) {
        /* First time through.  Initialize everything! */

        x = 0;
        y = 0;
        nBuf = 0;
        opened = 1;
        output[0] = 0;
    }

    do {
        if (x == nBuf && bzerror != BZ_STREAM_END && bzerror == BZ_OK &&
            opened) {
            /* Uncompress data into a static buffer */

            nBuf = BZ2_bzRead(&bzerror, bzfd, buffer, BZBUFFER);
            buffer[nBuf] = 0;
            x = 0;
        }

        /* Build a string from buffer contents */

        output[y] = buffer[x];

        if (output[y] == '\n' || output[y] == 0 || y == (int)length - 1) {
            output[y + 1] = 0;
            done = 1;
            y = 0;
        }

        else
            y++;
        x++;

    } while (done == 0);

    if (output[0] == 0)
        opened = 0;

    return (output);
}

char *SdfBz::GetString() { return BZfgets(bzfd, 255); }

bool SdfBz::OpenFile(std::string path) {
    if (!Sdf::OpenFile(path)) {
        return false;
    }

    bzfd = BZ2_bzReadOpen(&bzerror, fd, 0, 0, NULL, 0);

    return (bzerror == BZ_OK);
}

void SdfBz::CloseFile() {
    BZ2_bzReadClose(&bzerror, bzfd);

    Sdf::CloseFile();
}
