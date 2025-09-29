/**
 * @file sysutil.hpp
 *
 * @brief Miscellanous operating system utilities
 *
 * @author Michel Hoche-Mong
 *
 */

#pragma once

#include <cstddef>

#ifndef min
#define min(i, j) (i < j ? i : j)
#endif
#ifndef max
#define max(i, j) (i > j ? i : j)
#endif

#ifdef _WIN32
#define PATHSEP '\\'
#else
#define PATHSEP '/'
#endif

class SysUtil {
  public:
    /* memory utilities */
    static unsigned long long GetTotalSystemMemory();

    /* file name and path utilities */
    static char *Basename(char *path);
    static void StripFileExtension(char *path, char *ext, size_t extlen);
    static void ConvertBackslashes(char *path);
    static int SplitPath(const char *path, char *dir,
                         size_t dirNumberOfElements, char *fname,
                         size_t nameNumberOfElements, char *ext,
                         size_t extNumberOfElements);
    static char *CopyFilename(char *fname, const char *newSuffix);
};
