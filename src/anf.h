/** @file anf.h
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
#include "lrp.h"
#include "sdf.h"
#include "splat_run.h"

#include <string>

class Anf {
  private:
    const Lrp &lrp;
    const SplatRun &sr;

  public:
    /// <#Description#>
    /// @param lrp <#lrp description#>
    /// @param sr <#sr description#>
    Anf(const Lrp &lrp, const SplatRun &sr) : lrp(lrp), sr(sr) { }

    int LoadANO(const std::string &filename, Sdf &sdf, ElevationMap &em);
};
