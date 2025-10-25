/** @file sdf_bz.h
 *
 * File created by Peter Watkins (KE7IST) 4/31/20.
 * Derived from original project code.
 * Splat!
 * @copyright 1997 - 2018 John A. Magliacane (KD2BD) and contributors.
 * See revision control history for contributions.
 * This file is covered by the LICENSE.md file in the root of this project.
 */

#pragma once

#include "elevation_map.h"
#include "sdf.h"
#include "splat_run.h"
#include <bzlib.h>
#include <cstdio>
#include <string>

class SdfBz : public Sdf {
  private:
    int bzerror;
    BZFILE *bzfd;
    FILE *fd;  // Needed for bzip2 operations

  public:
    SdfBz(const std::string &path, const SplatRun &sr);

  protected:
    virtual bool OpenFile(std::string path);
    virtual void CloseFile();
    virtual bool GetString();

  private:
    char *BZfgets(BZFILE *bzfd, unsigned length);
};
