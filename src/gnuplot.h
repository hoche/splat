/** @file gnuplot.h
 *
 * File created by Peter Watkins (KE7IST) 1/8/18.
 * Derived from original project code.
 * Splat!
 * @copyright 1997 - 2018 John A. Magliacane (KD2BD) and contributors.
 * See revision control history for contributions.
 * This file is covered by the LICENSE.md file in the root of this project.
 */

#pragma once

#include "splat_run.h"
#include "path.h"
#include "site.h"
#include "elevation_map.h"
#include "lrp.h"

#include <stdio.h>

class GnuPlot {
  private:
    const SplatRun &sr;
    Path path;

  public:
    GnuPlot(const SplatRun &sr) : sr(sr), path(sr.arraysize, sr.ppd) {}

    void GraphTerrain(const Site &source, const Site &destination,
                      const std::string &name, const ElevationMap &em);

    void GraphElevation(const Site &source, const Site &destination,
                        const std::string &name, const ElevationMap &em);

    void GraphHeight(const Site &source, const Site &destination,
                     const std::string &name, bool fresnel_plot,
                     bool normalized, const ElevationMap &em, const Lrp &lrp);
};

