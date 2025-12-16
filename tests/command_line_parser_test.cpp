/** @file command_line_parser_test.cpp
 *
 * Unit tests for command-line argument parser
 * 
 * @copyright 1997 - 2018 John A. Magliacane (KD2BD) and contributors.
 */

#include "../src/command_line_parser.h"
#include "../src/splat_run.h"
#include <gtest/gtest.h>
#include <vector>

// Helper function to create argv array from vector of strings
std::vector<const char *> MakeArgv(const std::vector<std::string> &args) {
    std::vector<const char *> argv;
    for (const auto &arg : args) {
        argv.push_back(arg.c_str());
    }
    return argv;
}

// Test fixture for command-line parser
class CommandLineParserTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Initialize SplatRun with defaults
        sr.maxpages = 16;
        sr.arraysize = -1;
        sr.propagation_model = PROP_ITM;
        sr.hd_mode = false;
        sr.coverage = false;
        sr.LRmap = false;
        sr.terrain_plot = false;
        sr.elevation_plot = false;
        sr.height_plot = false;
        sr.map = false;
        sr.longley_plot = false;
        sr.norm = false;
        sr.topomap = false;
        sr.geo = false;
        sr.kml = false;
        sr.json = false;
        sr.pt2pt_mode = false;
        sr.area_mode = false;
        sr.ngs = false;
        sr.nolospath = false;
        sr.nositereports = false;
        sr.fresnel_plot = true;
        sr.command_line_log = false;
        sr.rxsite = false;
        sr.metric = true;
        sr.msl = false;
        sr.dbm = false;
        sr.bottom_legend = true;
        sr.smooth_contours = false;
        sr.altitude = 0.0;
        sr.altitudeLR = 0.0;
        sr.tx_range = 0.0;
        sr.rx_range = 0.0;
        sr.deg_range = 0.0;
        sr.deg_limit = 0.0;
        sr.max_range = 0.0;
        sr.clutter = 0.0;
        sr.forced_erp = -1.0;
        sr.forced_freq = 0.0;
        sr.fzone_clearance = 0.6;
        sr.contour_threshold = 0;
        sr.rx_site.lat = 91.0;
        sr.rx_site.lon = 361.0;
        sr.earthradius = EARTHRADIUS;
#ifdef HAVE_LIBPNG
        sr.imagetype = IMAGETYPE_PNG;
#else
        sr.imagetype = IMAGETYPE_PPM;
#endif
        sr.projection = PROJ_EPSG_4326;
        sr.multithread = true;
        sr.verbose = 1;
        sr.sdf_delimiter = "_";
    }

    SplatRun sr;
    CommandLineOptions options;
};

// Test help flag
TEST_F(CommandLineParserTest, ShowHelp) {
    std::vector<std::string> args = {"splat", "--help"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_FALSE(result);
    EXPECT_TRUE(options.show_help);
}

// Test no arguments
TEST_F(CommandLineParserTest, NoArguments) {
    std::vector<std::string> args = {"splat"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_FALSE(result);
    EXPECT_TRUE(options.show_help);
}

// Test -t (transmitter site) flag
TEST_F(CommandLineParserTest, TransmitterSingleSite) {
    std::vector<std::string> args = {"splat", "-t", "tx1.qth"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    ASSERT_EQ(options.tx_site_files.size(), 1);
    EXPECT_EQ(options.tx_site_files[0], "tx1.qth");
}

TEST_F(CommandLineParserTest, TransmitterMultipleSites) {
    std::vector<std::string> args = {"splat", "-t", "tx1.qth", "tx2.qth",
                                     "tx3.qth"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    ASSERT_EQ(options.tx_site_files.size(), 3);
    EXPECT_EQ(options.tx_site_files[0], "tx1.qth");
    EXPECT_EQ(options.tx_site_files[1], "tx2.qth");
    EXPECT_EQ(options.tx_site_files[2], "tx3.qth");
}

// Test -r (receiver site) flag
TEST_F(CommandLineParserTest, ReceiverSite) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-r", "rx.qth"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_EQ(options.rx_site_file, "rx.qth");
    EXPECT_TRUE(sr.rxsite);
    EXPECT_TRUE(sr.pt2pt_mode);
}

// Test -c (coverage) flag
TEST_F(CommandLineParserTest, CoverageMode) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-c", "10.0"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_TRUE(sr.coverage);
    EXPECT_TRUE(sr.map);
    EXPECT_TRUE(sr.area_mode);
    EXPECT_DOUBLE_EQ(sr.altitude, 10.0);
}

// Test -L (path loss map) flag
TEST_F(CommandLineParserTest, PathLossMap) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-L", "25.5"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_TRUE(sr.LRmap);
    EXPECT_TRUE(sr.map);
    EXPECT_TRUE(sr.area_mode);
    EXPECT_DOUBLE_EQ(sr.altitudeLR, 25.5);
}

// Test -s (city files) flag
TEST_F(CommandLineParserTest, CityFiles) {
    std::vector<std::string> args = {"splat", "-t",      "tx.qth", "-s",
                                     "city1.dat", "city2.dat"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    ASSERT_EQ(options.city_files.size(), 2);
    EXPECT_EQ(options.city_files[0], "city1.dat");
    EXPECT_EQ(options.city_files[1], "city2.dat");
}

// Test -b (boundary files) flag
TEST_F(CommandLineParserTest, BoundaryFiles) {
    std::vector<std::string> args = {"splat", "-t",     "tx.qth",     "-b",
                                     "bound1.dat", "bound2.dat", "bound3.dat"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    ASSERT_EQ(options.boundary_files.size(), 3);
    EXPECT_EQ(options.boundary_files[0], "bound1.dat");
    EXPECT_EQ(options.boundary_files[1], "bound2.dat");
    EXPECT_EQ(options.boundary_files[2], "bound3.dat");
}

// Test -p (terrain profile) flag
TEST_F(CommandLineParserTest, TerrainProfile) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-r", "rx.qth",
                                     "-p",     "terrain.png"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_EQ(options.terrain_file, "terrain.png");
    EXPECT_TRUE(sr.terrain_plot);
    EXPECT_TRUE(sr.pt2pt_mode);
}

// Test -e (elevation plot) flag
TEST_F(CommandLineParserTest, ElevationPlot) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-r", "rx.qth",
                                     "-e",     "elevation.png"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_EQ(options.elevation_file, "elevation.png");
    EXPECT_TRUE(sr.elevation_plot);
    EXPECT_TRUE(sr.pt2pt_mode);
}

// Test -h (height plot) flag
TEST_F(CommandLineParserTest, HeightPlot) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-r", "rx.qth",
                                     "-h",     "height.png"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_EQ(options.height_file, "height.png");
    EXPECT_TRUE(sr.height_plot);
    EXPECT_TRUE(sr.pt2pt_mode);
    EXPECT_FALSE(sr.norm);
}

// Test -H (normalized height plot) flag
TEST_F(CommandLineParserTest, NormalizedHeightPlot) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-r", "rx.qth",
                                     "-H",     "height_norm.png"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_EQ(options.height_file, "height_norm.png");
    EXPECT_TRUE(sr.height_plot);
    EXPECT_TRUE(sr.pt2pt_mode);
    EXPECT_TRUE(sr.norm);
}

// Test -l (longley plot) flag
TEST_F(CommandLineParserTest, LongleyPlot) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-r", "rx.qth",
                                     "-l",     "longley.png"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_EQ(options.longley_file, "longley.png");
    EXPECT_TRUE(sr.longley_plot);
    EXPECT_TRUE(sr.pt2pt_mode);
}

// Test -o (output map) flag
TEST_F(CommandLineParserTest, OutputMap) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-o", "output"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_EQ(options.mapfile, "output");
    EXPECT_TRUE(sr.map);
}

// Test -d (SDF directory) flag
TEST_F(CommandLineParserTest, SDFDirectory) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-d",
                                     "/path/to/sdf"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_EQ(sr.sdf_path, "/path/to/sdf");
}

// Test -m (earth radius multiplier) flag
TEST_F(CommandLineParserTest, EarthRadiusMultiplier) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-m", "1.333"};
    auto argv = MakeArgv(args);

    double original_radius = sr.earthradius;
    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_DOUBLE_EQ(sr.er_mult, 1.333);
    EXPECT_DOUBLE_EQ(sr.earthradius, original_radius * 1.333);
}

TEST_F(CommandLineParserTest, EarthRadiusMultiplierTooSmall) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-m", "0.05"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_DOUBLE_EQ(sr.er_mult, 1.0); // Should be clamped to 1.0
}

TEST_F(CommandLineParserTest, EarthRadiusMultiplierTooLarge) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-m", "2000000.0"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_DOUBLE_EQ(sr.er_mult, 1.0e6); // Should be clamped to 1e6
}

// Test -n (no LOS path) flag
TEST_F(CommandLineParserTest, NoLOSPath) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-n"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_TRUE(sr.nolospath);
}

// Test -N (no site reports) flag
TEST_F(CommandLineParserTest, NoSiteReports) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-N"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_TRUE(sr.nolospath);
    EXPECT_TRUE(sr.nositereports);
}

// Test -f (frequency) flag
TEST_F(CommandLineParserTest, Frequency) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-f", "915.0"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_DOUBLE_EQ(sr.forced_freq, 915.0);
}

TEST_F(CommandLineParserTest, FrequencyTooLow) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-f", "10.0"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_DOUBLE_EQ(sr.forced_freq, 0.0); // Should be set to 0
}

TEST_F(CommandLineParserTest, FrequencyTooHigh) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-f", "25000.0"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_DOUBLE_EQ(sr.forced_freq, 20.0e3); // Should be clamped to 20000
}

// Test -R (range) flag
TEST_F(CommandLineParserTest, Range) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-R", "50.0"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_DOUBLE_EQ(sr.max_range, 50.0);
}

TEST_F(CommandLineParserTest, RangeNegative) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-R", "-10.0"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_DOUBLE_EQ(sr.max_range, 0.0); // Should be clamped to 0
}

TEST_F(CommandLineParserTest, RangeTooLarge) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-R", "1500.0"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_DOUBLE_EQ(sr.max_range, 1000.0); // Should be clamped to 1000
}

// Test -v (verbosity) flag
TEST_F(CommandLineParserTest, Verbosity) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-v", "0"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_EQ(sr.verbose, 0);
}

// Test -st (single thread) flag
TEST_F(CommandLineParserTest, SingleThread) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-st"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_FALSE(sr.multithread);
}

// Test -hd (high definition) flag
TEST_F(CommandLineParserTest, HighDefinition) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-hd"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_TRUE(sr.hd_mode);
}

// Test -sc (smooth contours) flag
TEST_F(CommandLineParserTest, SmoothContours) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-sc"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_TRUE(sr.smooth_contours);
}

// Test -db (contour threshold) flag
TEST_F(CommandLineParserTest, ContourThreshold) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-db", "-100"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_EQ(sr.contour_threshold, -100);
}

TEST_F(CommandLineParserTest, ContourThresholdAlternateCase) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-dB", "-90"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_EQ(sr.contour_threshold, -90);
}

// Test -nf (no fresnel) flag
TEST_F(CommandLineParserTest, NoFresnel) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-nf"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_FALSE(sr.fresnel_plot);
}

// Test -fz (fresnel zone clearance) flag
TEST_F(CommandLineParserTest, FresnelZoneClearance) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-fz", "80"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_DOUBLE_EQ(sr.fzone_clearance, 0.8); // 80% -> 0.8
}

TEST_F(CommandLineParserTest, FresnelZoneClearanceNegative) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-fz", "-10"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_DOUBLE_EQ(sr.fzone_clearance, 0.6); // Should default to 60%
}

TEST_F(CommandLineParserTest, FresnelZoneClearanceTooLarge) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-fz", "150"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_DOUBLE_EQ(sr.fzone_clearance, 0.6); // Should default to 60%
}

// Test -gc (ground clutter) flag
TEST_F(CommandLineParserTest, GroundClutter) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-gc", "30.0"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_DOUBLE_EQ(sr.clutter, 30.0);
}

TEST_F(CommandLineParserTest, GroundClutterNegative) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-gc", "-5.0"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_DOUBLE_EQ(sr.clutter, 0.0); // Should be clamped to 0
}

// Test image format flags
#ifdef HAVE_LIBPNG
TEST_F(CommandLineParserTest, PPMFormat) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-ppm"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_EQ(sr.imagetype, IMAGETYPE_PPM);
}
#endif

#ifdef HAVE_LIBJPEG
TEST_F(CommandLineParserTest, JPGFormat) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-jpg"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_EQ(sr.imagetype, IMAGETYPE_JPG);
}
#endif

#ifdef HAVE_LIBGDAL
TEST_F(CommandLineParserTest, TIFFormat) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-tif"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_EQ(sr.imagetype, IMAGETYPE_GEOTIFF);
}
#endif

// Test -ngs (no greyscale) flag
TEST_F(CommandLineParserTest, NoGreyscale) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-ngs"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_TRUE(sr.ngs);
}

// Test -erp (effective radiated power) flag
TEST_F(CommandLineParserTest, ERP) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-erp", "1000.0"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_DOUBLE_EQ(sr.forced_erp, 1000.0);
}

TEST_F(CommandLineParserTest, ERPNegative) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-erp", "-10.0"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_DOUBLE_EQ(sr.forced_erp, -1.0); // Should be set to -1
}

// Test -ano (alphanumeric output) flag
TEST_F(CommandLineParserTest, AlphanumericOutput) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-ano",
                                     "output.txt"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_EQ(options.ano_filename, "output.txt");
}

// Test -ani (alphanumeric input) flag
TEST_F(CommandLineParserTest, AlphanumericInput) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-ani",
                                     "input.txt"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_EQ(options.ani_filename, "input.txt");
}

// Test -udt (user defined terrain) flag
TEST_F(CommandLineParserTest, UserDefinedTerrain) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-udt",
                                     "terrain.udt"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_EQ(options.udt_file, "terrain.udt");
}

// Test -kml flag
TEST_F(CommandLineParserTest, KMLOutput) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-kml"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_TRUE(sr.kml);
}

// Test -kmz flag
TEST_F(CommandLineParserTest, KMZOutput) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-kmz"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_TRUE(sr.kmz);
}

// Test -geo flag
TEST_F(CommandLineParserTest, GeoOutput) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-geo"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_TRUE(sr.geo);
}

// Test -dbm flag
TEST_F(CommandLineParserTest, DBMOutput) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-dbm"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_TRUE(sr.dbm);
}

// Test -log flag
TEST_F(CommandLineParserTest, LogOutput) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-log",
                                     "command.log"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_TRUE(sr.command_line_log);
    EXPECT_EQ(options.logfile, "command.log");
}

// Test -json flag
TEST_F(CommandLineParserTest, JSONOutput) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-json"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_TRUE(sr.json);
}

// Test -gpsav flag
TEST_F(CommandLineParserTest, GnuplotSave) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-gpsav"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_TRUE(sr.gpsav);
}

// Test -itwom flag
TEST_F(CommandLineParserTest, ITWOMModel) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-itwom"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_EQ(sr.propagation_model, PROP_ITWOM);
}

// Test -imperial flag
TEST_F(CommandLineParserTest, ImperialUnits) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-imperial"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_FALSE(sr.metric);
}

// Test -msl flag
TEST_F(CommandLineParserTest, MSLAltitude) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-msl"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_TRUE(sr.msl);
}

// Test -maxpages flag
TEST_F(CommandLineParserTest, MaxPages) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-maxpages", "25"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_EQ(sr.maxpages, 25);
}

TEST_F(CommandLineParserTest, MaxPagesInvalid) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-maxpages",
                                     "invalid"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_FALSE(result);
    EXPECT_TRUE(options.parse_error);
}

// Test -sdelim flag
TEST_F(CommandLineParserTest, SDFDelimiter) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-sdelim", ":"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_EQ(sr.sdf_delimiter, ":");
}

// Test validation - no transmitter sites
TEST_F(CommandLineParserTest, ValidateNoTransmitter) {
    bool result = ValidateCommandLine(sr, options);

    EXPECT_FALSE(result);
}

// Test validation - valid configuration
TEST_F(CommandLineParserTest, ValidateValidConfiguration) {
    options.tx_site_files.push_back("tx.qth");
    sr.maxpages = 16;

    bool result = ValidateCommandLine(sr, options);

    EXPECT_TRUE(result);
}

// Test validation - invalid maxpages
TEST_F(CommandLineParserTest, ValidateInvalidMaxPages) {
    options.tx_site_files.push_back("tx.qth");
    sr.maxpages = 7; // Invalid value

    bool result = ValidateCommandLine(sr, options);

    EXPECT_FALSE(result);
}

// Test validation - maxpages=1 without HD mode
TEST_F(CommandLineParserTest, ValidateMaxPages1WithoutHD) {
    options.tx_site_files.push_back("tx.qth");
    sr.maxpages = 1;
    sr.hd_mode = false;

    bool result = ValidateCommandLine(sr, options);

    EXPECT_FALSE(result);
}

// Test validation - maxpages=1 with HD mode
TEST_F(CommandLineParserTest, ValidateMaxPages1WithHD) {
    options.tx_site_files.push_back("tx.qth");
    sr.maxpages = 1;
    sr.hd_mode = true;

    bool result = ValidateCommandLine(sr, options);

    EXPECT_TRUE(result);
}

// Test multiple flags combined
TEST_F(CommandLineParserTest, MultipleFlagsCombined) {
    std::vector<std::string> args = {"splat", "-t",  "tx.qth",     "-r",
                                     "rx.qth", "-hd", "-itwom",     "-imperial",
                                     "-nf",    "-sc", "-maxpages", "36"};
    auto argv = MakeArgv(args);

    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_EQ(options.tx_site_files.size(), 1);
    EXPECT_EQ(options.rx_site_file, "rx.qth");
    EXPECT_TRUE(sr.hd_mode);
    EXPECT_EQ(sr.propagation_model, PROP_ITWOM);
    EXPECT_FALSE(sr.metric);
    EXPECT_FALSE(sr.fresnel_plot);
    EXPECT_TRUE(sr.smooth_contours);
    EXPECT_EQ(sr.maxpages, 36);
}

// Test conflicting options (coverage and path loss)
TEST_F(CommandLineParserTest, ConflictingCoverageAndPathLoss) {
    std::vector<std::string> args = {"splat", "-t", "tx.qth", "-c",
                                     "10.0",   "-L", "20.0"};
    auto argv = MakeArgv(args);

    // Parser allows both, but coverage takes precedence (L ignored with warning)
    bool result = ParseCommandLine(args.size(), argv.data(), sr, options);

    EXPECT_TRUE(result);
    EXPECT_TRUE(sr.coverage);
    EXPECT_TRUE(sr.LRmap);
}
