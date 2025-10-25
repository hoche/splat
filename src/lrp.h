/** @file lrp.h
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

#include <string>

class Lrp {
  private:
    double forced_erp;
    double forced_freq;

  public:
    Lrp(double forced_erp, double forced_freq)
        : forced_erp(forced_erp), forced_freq(forced_freq) {}

    double eps_dielect;
    double sgm_conductivity;
    double eno_ns_surfref;
    double frq_mhz;
    double conf;
    double rel;
    double erp;
    int radio_climate;
    int pol;

    char ReadLRParm(const Site &txsite, char forced_read, bool &loadPat,
                    std::string &filename);
};

