/** @file pat_file.h
 *
 * File created by Peter Watkins (KE7IST) 1/7/18.
 * Derived from original project code.
 * Splat!
 * @copyright 1997 - 2018 John A. Magliacane (KD2BD) and contributors.
 * See revision control history for contributions.
 * This file is covered by the LICENSE.md file in the root of this project.
 */

#pragma once

#include <string>

class AntennaPattern {
  public:
    bool got_elevation_pattern;
    bool got_azimuth_pattern;
    float antenna_pattern[361][1001];

  public:
    void LoadAntennaPattern(const std::string &filename);
};
