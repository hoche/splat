/** @file itwom3.0.h
 *
 * Derived from original project code.
 * Splat!
 * @copyright 1997 - 2018 John A. Magliacane (KD2BD) and contributors.
 * See revision control history for contributions.
 * This file is covered by the LICENSE.md file in the root of this project.
 */

#pragma once

#ifdef ITM_ELEV_DOUBLE
#define elev_t double
#else
#define elev_t float
#endif

double ITWOMVersion();

void point_to_point_ITM(const elev_t elev[], double tht_m, double rht_m,
                        double eps_dielect, double sgm_conductivity,
                        double eno_ns_surfref, double frq_mhz,
                        int radio_climate, int pol, double conf, double rel,
                        double &dbloss, char *strmode, int &errnum);

void point_to_point(const elev_t elev[], double tht_m, double rht_m,
                    double eps_dielect, double sgm_conductivity,
                    double eno_ns_surfref, double frq_mhz, int radio_climate,
                    int pol, double conf, double rel, double &dbloss,
                    char *strmode, int &errnum);
