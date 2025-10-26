/** @file kml.h
 *
 * File created by Peter Watkins (KE7IST) 1/9/18.
 * Derived from original project code.
 * Splat!
 * @copyright 1997 - 2018 John A. Magliacane (KD2BD) and contributors.
 * See revision control history for contributions.
 * This file is covered by the LICENSE.md file in the root of this project.
 */

#pragma once

#include <stdio.h>
#include <string>

#include "elevation_map.h"
#include "path.h"
#include "splat_run.h"

class Kml {
  private:
    Path path;
    const ElevationMap &em;
    const SplatRun &sr;

    std::string SanitizeFilename(const std::string &filename);
    void GenerateKMLContent(FILE *fd, const Site &source, const Site &destination,
                           double azimuth, double distance);

  public:
    Kml(const ElevationMap &em, const SplatRun &sr);
    void WriteKML(const Site &source, const Site &destination);
    void WriteKMZ(const Site &source, const Site &destination);
};
