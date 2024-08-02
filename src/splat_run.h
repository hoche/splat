/** @file splat_run.h
 *
 * File created by Peter Watkins (KE7IST) 6/17/15.
 * Derived from original project code.
 * Splat!
 * @copyright 1997 - 2018 John A. Magliacane (KD2BD) and contributors.
 * See revision control history for contributions.
 * This file is covered by the LICENSE.md file in the root of this project.
 */

#ifndef splat_run_h
#define splat_run_h

#include <string>
#include <iostream>

#include "imagewriter.h"
#include "site.h"

#define EARTHRADIUS 20902230.97
#define METERS_PER_MILE 1609.344
#define METERS_PER_FOOT 0.3048
#define KM_PER_MILE 1.609344
#define FOUR_THIRDS 1.3333333333333

const size_t SPLAT_VERSION_SIZE = 6;
const size_t SPLAT_NAME_SIZE = 10;
const size_t DASHES_SIZE = 80;
const size_t SDF_PATH_SIZE = 255;

typedef enum PropagationModel {
    PROP_ITM = 0,
    PROP_ITWOM
} PropagationModel;

class SplatRun {
  public:
    static const std::string splat_name;
    static const std::string splat_version;

    PropagationModel propagation_model;

    std::string sdf_path;

    double max_range;
    double forced_erp;
    double dpp;
    double ppd;
    double fzone_clearance;
    double clutter;

    double forced_freq;
    int contour_threshold;
    double earthradius;

    double altitude;
    double altitudeLR;
    double tx_range;
    double rx_range;
    double deg_range;
    double deg_limit;
    double deg_range_lon;
    double er_mult;

    Site rx_site;

    int ippd;
    int maxpages;
    int mpi;
    int max_txsites;
    int arraysize;

    bool kml;
    bool json;
    bool geo;
    bool gpsav;
    bool ngs;
    bool fresnel_plot;
    bool nolospath;
    bool nositereports;
    bool hd_mode;

    bool coverage;
    bool LRmap;
    bool terrain_plot;
    bool elevation_plot;
    bool height_plot;
    bool map;
    bool longley_plot;
    bool norm;
    bool topomap;
    bool pt2pt_mode;
    bool area_mode;
    bool command_line_log;
    bool rxsite;

    bool metric;
    bool old_longitude;
    bool dbm;
    bool smooth_contours;
    bool bottom_legend;
    bool verbose;
    bool multithread;
    std::string sdf_delimiter;
    ImageType imagetype;
    ProjectionType projection;
};

#endif /* splat_run_h */
