/** @file path.h
 *
 * File created by Peter Watkins (KE7IST) 4/8/16.
 * Derived from original project code.
 * Splat!
 * @copyright 1997 - 2018 John A. Magliacane (KD2BD) and contributors.
 * See revision control history for contributions.
 * This file is covered by the LICENSE.md file in the root of this project.
 */

#pragma once

#include "site.h"

#include <vector>

// Forward-declare ElevationMap since ElevationMap uses Path
class ElevationMap;

class Path {
  private:
    double ppd;
    int arraysize;

  public:
    std::vector<double> lat;
    std::vector<double> lon;
    std::vector<double> elevation;
    std::vector<double> distance;
    int length;

  public:
    Path(int size, double ppd)
        : ppd(ppd), arraysize(size), lat(std::vector<double>(size)),
          lon(std::vector<double>(size)), elevation(std::vector<double>(size)),
          distance(std::vector<double>(size)), length(0) {}

    void ReadPath(const Site &source, const Site &destination,
                  const ElevationMap &em);

    ~Path();
};

