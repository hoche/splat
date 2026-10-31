/** @file mapterhorn.cpp
 *
 * Mapterhorn terrain source support for SPLAT!
 *
 * @copyright 1997 - 2018 John A. Magliacane (KD2BD) and contributors.
 * See revision control history for contributions.
 * This file is covered by the LICENSE.md file in the root of this project.
 */

#include "mapterhorn.h"
#include "utilities.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <set>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#ifndef _WIN32
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace {

const char *const kDefaultSource =
    "https://download.mapterhorn.com/planet.pmtiles";

struct MtPage {
    int min_north;
    int min_west;
    int max_west;
};

bool IsRemote(const std::string &s) {
    return s.rfind("http://", 0) == 0 || s.rfind("https://", 0) == 0;
}

std::string EffectiveSource(const SplatRun &sr) {
    return sr.mt_source.empty() ? std::string(kDefaultSource) : sr.mt_source;
}

int EffectiveZoom(const SplatRun &sr) {
    if (sr.mt_zoom > 0)
        return sr.mt_zoom;
    return sr.hd_mode ? 12 : 10;
}

/* SDF pages are named by degrees west; mapterhorn2sdf is addressed by the
 * integer longitude (degrees east) of the south-west corner of the page. */
int SouthWestLonEast(int max_west) {
    return (max_west <= 180) ? -max_west : 360 - max_west;
}

std::string PageName(const SplatRun &sr, const MtPage &p) {
    const std::string &d = sr.sdf_delimiter;
    return std::to_string(p.min_north) + d + std::to_string(p.min_north + 1) +
           d + std::to_string(p.min_west) + d + std::to_string(p.max_west) +
           (sr.hd_mode ? "-hd" : "");
}

bool PageExists(const std::string &name, const fs::path &cache) {
    std::error_code ec;
    for (const char *suffix : {".sdf", ".sdf.bz2"}) {
        /* SPLAT! looks in the working directory before the SDF path */
        if (fs::exists(fs::path(name + suffix), ec))
            return true;
        if (fs::exists(cache / (name + suffix), ec))
            return true;
    }
    return false;
}

/* Same page enumeration as ElevationMap::LoadTopoData() */
std::vector<MtPage> EnumeratePages(int max_lon, int min_lon, int max_lat,
                                   int min_lat) {
    std::vector<MtPage> pages;
    std::set<std::pair<int, int>> seen;

    int x, y, width, ymin, ymax;

    width = Utilities::ReduceAngle(max_lon - min_lon);

    for (y = 0; y <= width; y++)
        for (x = min_lat; x <= max_lat; x++) {
            if ((max_lon - min_lon) <= 180.0)
                ymin = (int) (min_lon + (double) y);
            else
                ymin = max_lon + y;

            while (ymin < 0)
                ymin += 360;

            while (ymin >= 360)
                ymin -= 360;

            ymax = ymin + 1;

            while (ymax < 0)
                ymax += 360;

            while (ymax >= 360)
                ymax -= 360;

            if (seen.insert(std::make_pair(x, ymin)).second)
                pages.push_back({x, ymin, ymax});
        }

    return pages;
}

#ifndef _WIN32
bool IsExecutable(const fs::path &p) {
    std::error_code ec;
    return fs::is_regular_file(p, ec) && access(p.c_str(), X_OK) == 0;
}
#endif

/* Locates the mapterhorn2sdf executable. Returns an empty string if not
 * found. */
std::string FindMapterhorn2Sdf(const SplatRun &sr) {
#ifdef _WIN32
    (void) sr;
    return std::string();
#else
    if (!sr.mapterhorn2sdf_path.empty())
        return sr.mapterhorn2sdf_path;

    const char *env = getenv("SPLAT_MAPTERHORN2SDF");
    if (env && *env)
        return env;

    std::error_code ec;
    fs::path self = fs::read_symlink("/proc/self/exe", ec);
    if (!ec) {
        fs::path candidate = self.parent_path() / "mapterhorn2sdf";
        if (IsExecutable(candidate))
            return candidate.string();
    }

    const char *path_env = getenv("PATH");
    if (path_env) {
        std::string path = path_env;
        size_t start = 0;
        while (start <= path.size()) {
            size_t end = path.find(':', start);
            if (end == std::string::npos)
                end = path.size();
            std::string dir = path.substr(start, end - start);
            if (dir.empty())
                dir = ".";
            fs::path candidate = fs::path(dir) / "mapterhorn2sdf";
            if (IsExecutable(candidate))
                return candidate.string();
            start = end + 1;
        }
    }

    return std::string();
#endif
}

/* Runs a program with the given arguments (argv[0] included) and waits for
 * it. Returns true if it exited with status 0. */
bool RunProgram(const std::vector<std::string> &args) {
#ifdef _WIN32
    (void) args;
    return false;
#else
    std::vector<char *> argv;
    for (const std::string &a : args)
        argv.push_back(const_cast<char *>(a.c_str()));
    argv.push_back(nullptr);

    fflush(stdout);
    fflush(stderr);

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return false;
    }

    if (pid == 0) {
        execv(argv[0], argv.data());
        perror(argv[0]);
        _exit(127);
    }

    int status = 0;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno != EINTR) {
            perror("waitpid");
            return false;
        }
    }

    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
#endif
}

} // namespace

void SetupMapterhorn(SplatRun &sr) {
    std::string cache = sr.mt_cache;

    if (cache.empty())
        cache = sr.sdf_path;

    if (cache.empty())
        cache = "./sdf-cache";

    if (*cache.rbegin() != '/')
        cache += '/';

    sr.mt_cache = cache;
    sr.sdf_path = cache;
}

bool EnsureMapterhornPages(const SplatRun &sr, int max_lon, int min_lon,
                           int max_lat, int min_lat) {
    const fs::path cache(sr.mt_cache.empty() ? std::string("./sdf-cache")
                                             : sr.mt_cache);
    const std::string source = EffectiveSource(sr);

    std::vector<MtPage> missing;
    for (const MtPage &p : EnumeratePages(max_lon, min_lon, max_lat, min_lat)) {
        /* Mapterhorn covers latitudes -85 to 85 only; SPLAT! treats pages
         * without a file as sea level. */
        if (p.min_north < -85 || p.min_north > 84)
            continue;

        if (!PageExists(PageName(sr, p), cache))
            missing.push_back(p);
    }

    if (missing.empty())
        return true;

    if (sr.mt_offline && IsRemote(source)) {
        std::cerr << "\n*** ERROR: -mt-offline is set, the source is remote ("
                  << source << ") and these SDF pages are missing from "
                  << cache.string() << ":\n";
        for (const MtPage &p : missing)
            std::cerr << "    " << PageName(sr, p) << ".sdf\n";
        std::cerr << "\n";
        return false;
    }

    std::error_code ec;
    fs::create_directories(cache, ec);
    if (ec) {
        std::cerr << "\n*** ERROR: Cannot create Mapterhorn cache directory \""
                  << cache.string() << "\": " << ec.message() << "\n\n";
        return false;
    }

    const std::string tool = FindMapterhorn2Sdf(sr);
    if (tool.empty()) {
        std::cerr << "\n*** ERROR: mapterhorn2sdf was not found. Install it "
                     "next to the splat executable or in PATH, or set "
                     "SPLAT_MAPTERHORN2SDF.\n\n";
        return false;
    }

    for (const MtPage &p : missing) {
        std::vector<std::string> args;
        args.push_back(tool);
        args.push_back("--outdir");
        args.push_back(cache.string());
        if (sr.hd_mode)
            args.push_back("--hd");
        args.push_back("--zoom");
        args.push_back(std::to_string(EffectiveZoom(sr)));
        args.push_back("--source");
        args.push_back(source);
        if (sr.mt_offline)
            args.push_back("--offline");
        args.push_back("--delimiter");
        args.push_back(sr.sdf_delimiter);
        args.push_back(std::to_string(p.min_north));
        args.push_back(std::to_string(SouthWestLonEast(p.max_west)));

        if (!RunProgram(args)) {
            std::cerr << "\n*** ERROR: mapterhorn2sdf failed for page "
                      << PageName(sr, p) << "\n\n";
            return false;
        }

        if (!PageExists(PageName(sr, p), cache)) {
            std::cerr << "\n*** ERROR: mapterhorn2sdf did not produce "
                      << PageName(sr, p) << ".sdf in " << cache.string()
                      << "\n\n";
            return false;
        }
    }

    return true;
}
