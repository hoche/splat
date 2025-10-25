/** @file udt.h
 *
 * File created by Peter Watkins (KE7IST) 1/9/18.
 * Derived from original project code.
 * Splat!
 * @copyright 1997 - 2018 John A. Magliacane (KD2BD) and contributors.
 * See revision control history for contributions.
 * This file is covered by the LICENSE.md file in the root of this project.
 */

#pragma once

#include "elevation_map.h"
#include "splat_run.h"

#include <string>

class Udt {
  private:
    const SplatRun &sr;

  public:
    Udt(const SplatRun &sr) : sr(sr) { }

    void LoadUDT(const std::string &filename, ElevationMap &em);
};
