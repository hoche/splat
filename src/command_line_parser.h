/** @file command_line_parser.h
 *
 * Command-line argument parser for SPLAT!
 * Extracts and validates command-line arguments into SplatRun configuration
 * 
 * @copyright 1997 - 2018 John A. Magliacane (KD2BD) and contributors.
 * See revision control history for contributions.
 * This file is covered by the LICENSE.md file in the root of this project.
 */

#ifndef COMMAND_LINE_PARSER_H
#define COMMAND_LINE_PARSER_H

#include "splat_run.h"
#include "site.h"
#include <string>
#include <vector>

/**
 * Structure to hold parsed command-line options
 */
struct CommandLineOptions {
    // File paths
    std::vector<std::string> tx_site_files;
    std::string rx_site_file;
    std::vector<std::string> city_files;
    std::vector<std::string> boundary_files;
    std::string mapfile;
    std::string elevation_file;
    std::string height_file;
    std::string longley_file;
    std::string terrain_file;
    std::string udt_file;
    std::string ani_filename;
    std::string ano_filename;
    std::string logfile;
    
    // SplatRun configuration (will be populated into sr)
    // Most settings will be set directly in sr parameter
    
    // Parse status
    bool show_help = false;
    bool parse_error = false;
    std::string error_message;
};

/**
 * Parse command-line arguments and populate SplatRun configuration
 * 
 * @param argc Number of command-line arguments
 * @param argv Array of command-line argument strings
 * @param sr SplatRun object to populate with parsed settings
 * @param options CommandLineOptions structure to populate with file paths
 * @return true if parsing was successful, false if error or help requested
 */
bool ParseCommandLine(int argc, const char *argv[], SplatRun &sr,
                      CommandLineOptions &options);

/**
 * Validate parsed command-line options for consistency
 * 
 * @param sr SplatRun configuration
 * @param options Parsed command-line options
 * @return true if validation passed, false otherwise
 */
bool ValidateCommandLine(const SplatRun &sr, const CommandLineOptions &options);

/**
 * Print help message showing available command-line options
 */
void PrintHelp(const SplatRun &sr);

#endif // COMMAND_LINE_PARSER_H
