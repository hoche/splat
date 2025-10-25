/** @file report.h
 *
 * File created by Peter Watkins (KE7IST) 1/8/18.
 * Derived from original project code.
 * Splat!
 * @copyright 1997 - 2018 John A. Magliacane (KD2BD) and contributors.
 * See revision control history for contributions.
 * This file is covered by the LICENSE.md file in the root of this project.
 */

#pragma once

#include "antenna_pattern.h"
#include "elevation_map.h"
#include "path.h"
#include "site.h"
#include "splat_run.h"

#include <string>

class Report {
  private:
    const std::string dashes;

    const ElevationMap &em;
    const SplatRun &sr;
    Path path;

  public:
    Report(const ElevationMap &em, const SplatRun &sr)
        : dashes("-------------------------------------------------------------"
                 "--------------"),
          em(em),
          sr(sr),
          path(sr.arraysize, sr.ppd) { }

    void PathReport(const Site &source, const Site &destination,
                    const std::string &name, bool graph_it, elev_t elev[],
                    const AntennaPattern &pat, const Lrp &lrp);

    void SiteReport(const Site &xmtr);

  private:
    void ObstructionAnalysis(const Site &xmtr, const Site &rcvr, double f,
                             FILE *outfile);
};
