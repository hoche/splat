/** @file command_line_parser.h
 *
 * Command-line argument parser for SPLAT! using CLI11
 * Extracts and validates command-line arguments into SplatRun configuration
 * 
 * @copyright 1997 - 2018 John A. Magliacane (KD2BD) and contributors.
 * See revision control history for contributions.
 * This file is covered by the LICENSE.md file in the root of this project.
 */

#ifndef COMMAND_LINE_PARSER_H
#define COMMAND_LINE_PARSER_H

#include "site.h"
#include "splat_run.h"
#include <CLI/CLI.hpp>
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

    // Parse status
    bool show_help = false;
    bool parse_error = false;
    std::string error_message;
};

/**
 * Setup CLI11 application with all SPLAT! command-line options
 * 
 * @param app CLI11 app object to configure
 * @param sr SplatRun object to populate with parsed settings
 * @param options CommandLineOptions structure to populate with file paths
 */
void SetupCLI11(CLI::App &app, SplatRun &sr, CommandLineOptions &options);

/**
 * Parse command-line arguments using CLI11
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

#endif  // COMMAND_LINE_PARSER_H
