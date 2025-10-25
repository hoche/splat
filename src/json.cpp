/** @file jsonl.cpp
 *
 * File created by Stefan Erhardt (DL1NFS) 3/8/2020.
 * Derived from original project code.
 * Splat!
 * @copyright 1997 - 2018 John A. Magliacane (KD2BD) and contributors.
 * See revision control history for contributions.
 * This file is covered by the LICENSE.md file in the root of this project.
 */

#include "json.h"
#include "elevation_map.h"
#include "path.h"
#include "site.h"
#include "splat_run.h"
#include "utilities.h"
#include <iostream>
#include <fstream>

Json::Json(const ElevationMap &em, const SplatRun &sr)
    : path(sr.arraysize, sr.ppd), em(em), sr(sr) {}
    
// === unsorted start ===
    // JSON output by der-stefan
    // This is NOT yet for productive use! I currently test coverage predictions only, no point2point predictions.
    
    /* read in argv[] as an associative array.
	 * You can now access the list e.g. by args["o"] which will not fail if the argument is not set
	 * 
	 *	map<string, string>::iterator i;
	 *		for (i=args.begin(); i != args.end(); i++) {
	 *		cout << i->first << ": " << i->second << endl;
	 *	}
	 */
	//typedef map<string,string> arg_t;
/*	arg_t args;
	
	string curr_arg = "";
	int curr_arg_i = 0;
	for(int i=0; i<argc; i++) {		// step through argv[] array
		string arg = argv[i];
		if(arg.find("-") == 0) {	// check if current argument has leading "-"
			curr_arg = arg.erase(0,1);	// remove leading "-" and save as new array entry
			curr_arg_i = i;		// save position for multiple parameters
			args[curr_arg] = "";
		} else {
			if (curr_arg != "") {
				if(i == (curr_arg_i + 1)) {
					args[curr_arg] = arg;	// if no "-" was found the current argument is considered as a value to the previous argument
				} else {
					args[curr_arg] += " " + arg;	// if no "-" was found the current argument is considered as a value to the previous argument
				}
			}
		}
	}*/
	/* end argv[] reading */
	
    //Json json(*em_p, sr);
    //json.WriteJSON(args, tx_site[0], lrp, mapfile);
// === unsorted end ===

    
void Json::WriteJSON(arg_t args, Site tx_site, Lrp lrp, std::string mapfile) {
	std::string report_name;

    report_name = mapfile + ".json";

    for (size_t i = 0; i < report_name.length(); i++)
        if (report_name[i] == 32 || report_name[i] == 17 ||
            report_name[i] == 92 || report_name[i] == 42 ||
            report_name[i] == 47)
            report_name[i] = '_';

    std::ofstream reportfile(report_name);
    
    reportfile << "{\n";
    reportfile << "\t\"splat\": \"" << sr.splat_version.c_str() << "\",\n";
    reportfile << "\t\"name\": \"" << tx_site.name.c_str() << "\",\n";
    reportfile << "\t\"image\": {\n";
    reportfile << "\t\t\"file\": \"" << mapfile.c_str() << ".png\",\n";
    reportfile << "\t\t\"projection\": \"EPSG:3857\",\n";
    reportfile << "\t\t\"bounds\": [[" << em.min_north << ", " << 360-em.max_west << "],[" << em.max_north << ", " << 360-em.min_west << "]],\n";
    reportfile << "\t\t\"unit\": \"dbm\",\n";	//TODO: determine unit (dBm or dBuV/m)
    reportfile << "\t\t\"colormap\": {\n";
    reportfile << "\t\t\t\"0\": \"#FF0000\",\n";
    reportfile << "\t\t\t\"-10\": \"#FF8000\"\n";	//TODO: determine colormap
    reportfile << "\t\t}\n\t},\n";
    reportfile << "\t\"qth\": {\n";
    reportfile << "\t\t\"coordinates\": [" << tx_site.lat << ", " << 360-tx_site.lon << "],\n";
    reportfile << "\t\t\"height\": " << tx_site.alt << "\n";
    reportfile << "\t},\n";
    reportfile << "\t\"lrp\": {\n";
    reportfile << "\t\t\"permittivity\": " << lrp.eps_dielect << ",\n";
    reportfile << "\t\t\"conductivity\": " << lrp.sgm_conductivity << ",\n";
    reportfile << "\t\t\"bending\": " << lrp.eno_ns_surfref << ",\n";
    reportfile << "\t\t\"frequency\": " << lrp.frq_mhz << ",\n";
    reportfile << "\t\t\"climate\": " << lrp.radio_climate << ",\n";
    reportfile << "\t\t\"polarization\": " << lrp.pol << ",\n";
    reportfile << "\t\t\"location_variability\": " << lrp.conf << ",\n";
    reportfile << "\t\t\"time_variability\": " << lrp.rel << ",\n";
    reportfile << "\t\t\"erp\": " << lrp.erp << "\n";
    reportfile << "\t},\n";
    reportfile << "\t\"arguments\": {\n";

    std::map<std::string, std::string>::iterator i;
    int pos = 0;
    int len = args.size();
    for (i=args.begin(); i != args.end(); i++, pos++) {
		reportfile << "\t\t\"" <<  i->first.c_str() << "\": \"" << i->second.c_str() << "\"";
		if(pos < (len - 1)) {
			reportfile << ",\n";
		} else {
			reportfile << "\n";	// no comma for last array entry
		}
	}

    reportfile << "\t}\n";
    reportfile << "}\n";

	reportfile.close();

	std::cout << "\nJSON file written to: " << report_name;

    fflush(stdout);
}
