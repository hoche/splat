/** @file splat_run.cpp
 *
 * File created by Peter Watkins (KE7IST) 6/17/15.
 * Derived from original project code.
 * Splat!
 * @copyright 1997 - 2018 John A. Magliacane (KD2BD) and contributors.
 * See revision control history for contributions.
 * This file is covered by the LICENSE.md file in the root of this project.
 */

#include "splat_run.h"
#include "dem.h"
#include "itwom3.0.h"
#include "path.h"
#include "site.h"
#include <bzlib.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

const std::string SplatRun::splat_name = "SPLAT!";
const std::string SplatRun::splat_version = "2.0-alpha";
