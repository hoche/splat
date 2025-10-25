/** @file region.h
 *
 * File created by Peter Watkins (KE7IST) 1/8/18.
 * Derived from original project code.
 * Splat!
 * @copyright 1997 - 2018 John A. Magliacane (KD2BD) and contributors.
 * See revision control history for contributions.
 * This file is covered by the LICENSE.md file in the root of this project.
 */

#pragma once

#include "site.h"

class Region {
  public:
    unsigned char color[32][3];
    int level[32];
    int levels;

    void LoadLossColors(const Site &xmtr);
    void LoadDBMColors(const Site &xmtr);
    void LoadSignalColors(const Site &xmtr);
};

