/** @file mapterhorn.h
 *
 * Mapterhorn terrain source support for SPLAT!
 *
 * SDF files are generated on demand by the external mapterhorn2sdf utility
 * and are then loaded through the regular SDF code path.
 *
 * @copyright 1997 - 2018 John A. Magliacane (KD2BD) and contributors.
 * See revision control history for contributions.
 * This file is covered by the LICENSE.md file in the root of this project.
 */

#pragma once

#include "splat_run.h"

/// Resolves the directory used for generated SDF files and makes it the SDF
/// search path. The cache is -mt-cache if given, else the SDF directory (-d or
/// ~/.splat_path), else ./sdf-cache. Must be called after the SDF path is
/// known and before the Sdf object is constructed.
/// @param sr The run configuration. sr.mt_cache and sr.sdf_path are updated.
void SetupMapterhorn(SplatRun &sr);

/// Makes sure that every 1x1 degree page needed to cover the given region
/// exists as an SDF file in the Mapterhorn cache, generating missing pages
/// with mapterhorn2sdf. The region arguments are the same as those given to
/// ElevationMap::LoadTopoData().
/// @return true if all pages are available (or intentionally skipped because
/// they lie outside the Mapterhorn coverage), false on error.
bool EnsureMapterhornPages(const SplatRun &sr, int max_lon, int min_lon,
                           int max_lat, int min_lat);
