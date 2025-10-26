/** @file dem.h
 *
 * File created by Peter Watkins (KE7IST) 4/8/16.
 * Derived from original project code.
 * Splat!
 * @copyright 1997 - 2018 John A. Magliacane (KD2BD) and contributors.
 * See revision control history for contributions.
 * This file is covered by the LICENSE.md file in the root of this project.
 */

#pragma once

#include <vector>

class Dem {
  public:
    int min_north;
    int max_north;
    int min_west;
    int max_west;
    int max_el;
    int min_el;
    std::vector<short> data;
    std::vector<unsigned char> mask;
    std::vector<unsigned char> signal;

  public:
    Dem(int size)
        : min_north(0),
          max_north(0),
          min_west(0),
          max_west(0),
          max_el(0),
          min_el(0),
          data(std::vector<short>(size * size)),
          mask(std::vector<unsigned char>(size * size)),
          signal(std::vector<unsigned char>(size * size)) { }

    ~Dem();
};
