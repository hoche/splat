/** @file json.h
 *
 * File created by Stefan Erhardt (DL1NFS) 3/8/2020.
 * Derived from original project code.
 * Splat!
 * @copyright 1997 - 2018 John A. Magliacane (KD2BD) and contributors.
 * See revision control history for contributions.
 * This file is covered by the LICENSE.md file in the root of this project.
 */

#ifndef json_h
#define json_h

#include "elevation_map.h"
#include "path.h"
#include "splat_run.h"
#include <map>

typedef std::map<std::string,std::string> arg_t;

class Json {
  private:
    Path path;
    const ElevationMap &em;
    const SplatRun &sr;

  public:
    Json(const ElevationMap &em, const SplatRun &sr);
    void WriteJSON(arg_t, Site, Lrp, std::string);
};


#endif /* json_h */
