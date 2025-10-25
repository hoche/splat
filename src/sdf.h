/** @file sdf.h
 *
 * File created by Peter Watkins (KE7IST) 1/8/18.
 * Derived from original project code.
 * Splat!
 * @copyright 1997 - 2018 John A. Magliacane (KD2BD) and contributors.
 * See revision control history for contributions.
 * This file is covered by the LICENSE.md file in the root of this project.
 */

#pragma once

#include "elevation_map.h"
#include "splat_run.h"
#include "dem.h"

#include <string>

class Sdf {
  private:
    std::string sdf_path;
    const SplatRun &sr;
    char line[20];

  protected:
    std::string suffix;
    FILE *fd;

  public:
    Sdf(const std::string &path, const SplatRun &sr)
        : sdf_path(path), sr(sr), suffix(".sdf") {}

    int LoadSDF(ElevationMap &em, const std::string &name, int minlat,
                int maxlat, int minlon, int maxlon);
    char LoadSDF(ElevationMap &em, int minlat, int maxlat, int minlon,
                 int maxlon);

  protected:
    virtual bool OpenFile(std::string path);
    virtual void CloseFile();
    virtual char *GetString();

  private:
    Dem *FindEmptyDem(ElevationMap &em, int minlat, int maxlat, int minlon,
                      int maxlon, int &indx);
};

