/** @file image.h
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
#include "imagewriter.h"
#include "region.h"
#include "site.h"
#include "splat_run.h"
#include "utilities.h"

#include <string>
#include <vector>

typedef enum MapType {
    MAPTYPE_DBM,
    MAPTYPE_DBUVM,
    MAPTYPE_PATHLOSS,
    MAPTYPE_LOS
} MapType;

class Image {
  private:
    const SplatRun &sr;
    const ElevationMap &em;
    std::string &filename;
    const std::vector<Site> &xmtr;
    const double one_over_gamma = 1.0 / GAMMA;
    double conversion;

  public:
    Image(const SplatRun &sr, std::string &filename,
          const std::vector<Site> &xmtr, const ElevationMap &em);

    void WriteCoverageMap(MapType maptype, ImageType imagetype, Region &region);

  private:
    void WriteKmlForImage(const std::string &groundOverlayName,
                          const std::string &description,
                          bool writeScreenOverlay, const std::string &kmlfile,
                          const std::string &mapfile, double north,
                          double south, double east, double west,
                          const std::string &ckfile);
    static void WriteGeo(const std::string &geofile, const std::string &mapfile,
                         double north, double south, double east, double west,
                         unsigned int width, unsigned int height);
    Pixel GetPixel(const Dem *dem, MapType maptype, Region &region, int x0,
                   int y0);

    int GetIndexForLegend(int colorwidth, MapType maptype, Region &region,
                          int x0, int y0);

    int GetIndexForColorKeyImageFile(MapType maptype, Region &region, int x0,
                                     int y0);

    void WriteColorKeyImageFile(const std::string &ckfile, ImageType imagetype,
                                MapType maptype, Region &region);

    void WriteLegend(ImageWriter &iw, MapType maptype, Region &region,
                     unsigned int width);
};
