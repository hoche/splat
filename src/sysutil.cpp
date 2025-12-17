/** @file sysutil.cpp
 *
 * Derived from original project code.
 * Splat!
 * @copyright 1997 - 2018 John A. Magliacane (KD2BD) and contributors.
 * See revision control history for contributions.
 * This file is covered by the LICENSE.md file in the root of this project.
 */

#include "sysutil.h"

#include <cerrno>
#include <cstddef>
#include <cstdio>  // for snprintf
#include <cstdlib>
#include <cstring>

#ifndef _WIN32
#include <libgen.h>  // for dirname() and basename()
#endif

#ifndef _WIN32
#include <unistd.h>

unsigned long long SysUtil::GetTotalSystemMemory() {
    unsigned long pages = sysconf(_SC_PHYS_PAGES);
    unsigned long page_size = sysconf(_SC_PAGE_SIZE);
    return pages * page_size;
}
#else
#include <windows.h>

unsigned long long SysUtil::GetTotalSystemMemory() {
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);
    return status.ullTotalPhys;
}
#endif

char *SysUtil::Basename(char *path) {
    if (! path)
        return (char *) ""; /* const string */

    int i = (int) strlen(path);
    for (; i >= 0 && i != '/' && i != '\\'; --i)
        ; /* walk backwards until we find a path separator */
    ++i;  /* i is either -1 or the index of the slash, so move forward */
    return &(path[i]);
}

/* Strip the extension off a path
 *    path: path to strip
 *     ext: buffer to hold the extension
 *  extlen: size of ext INCLUDING the null byte
 */
void SysUtil::StripFileExtension(char *path, char *ext, size_t extlen) {
    char *p;
    size_t len;

    p = strrchr(path, '.');

    if (p == NULL) {
        ext[0] = '\0';
        return;
    }

    *p++ = '\0'; /* chop off extension and move ahead */

    len = min(strlen(p) + 1, extlen);
    memcpy(ext, p, len);
    ext[len] = '\0';
}

// In-place conversion of backslashes to forward slashes
void SysUtil::ConvertBackslashes(char *path) {
    size_t i;
    for (i = strlen(path) - 1; i; --i) {
        if (path[i] == '\\')
            path[i] = '/';
    }
}

/* Windows has a useful function called _splitpath_s(). This is
 * intended to replicate that. */
int SysUtil::SplitPath(const char *path, char *dir, size_t dirNumberOfElements,
                       char *fname, size_t nameNumberOfElements, char *ext,
                       size_t extNumberOfElements) {
    char *p;

    if (path == NULL) {
        return EINVAL;
    }

    char *fullpath = strdup(path);

    /* Peel off everything up to (but not including) the final slash.
     * If there is no slash (local file), returns an empty string.
     * Guaranteed to be NULL terminated.
     */
    if (dir != NULL) {
        char *tmp = strdup(fullpath); /* dirname munges its arg so copy*/
        char *dname = dirname(tmp);   /* If there is no slash, gets ".". */
        if (dirNumberOfElements < strlen(dname) + 1) {
            free(tmp);
            free(fullpath);
            return ERANGE;
        }
        if (dname[0] == '.' && dname[1] == '\0') {
            dir[0] = '\0';
        } else {
            memcpy(dir, dname, strlen(dname) + 1);
        }
        free(tmp);
    }

    /* Everything after the last slash, or the whole thing if
     * there is no slash. guaranteed to be null-terminated.
     */
    char *base = basename(fullpath);

    if (((p = strchr(base, '.')) == NULL) || (strlen(p) < 2)) {
        /* Easy case: no suffix */
        if (ext) {
            if (extNumberOfElements < 1) {
                free(fullpath);
                return ERANGE;
            }
            ext[0] = '\0';
        }
    } else {
        /* peel off everything from the first dot to the end. Note
         * that multiple suffixes get treated as one; i.e. "foo.tar.gz"
         * gets split into "foo" and "tar.gz".
         */
        *p++ = '\0';
        if (ext) {
            if (extNumberOfElements < strlen(p)) {
                free(fullpath);
                return ERANGE;
            }
            memcpy(ext, p, strlen(p) + 1);
        }
    }

    if (fname) {
        if (nameNumberOfElements < strlen(base) + 1) {
            free(fullpath);
            return ERANGE;
        }
        memcpy(fname, base, strlen(base) + 1);
    }

    free(fullpath);
    return 0;
}

/* Copy a filename, up to but not including the suffix.
 * If newSuffix is specified, add that on.
 * The new filename is malloced and must be freed when
 * the caller is done with it.
 */
char *SysUtil::CopyFilename(char *fname, const char *newSuffix) {
    char pathbuf[1027], filebuf[512], suffixbuf[64];
    char *newFname;

#ifdef _WIN32
    char drivebuf[3];
    drivebuf[0] = '\0';
    if (_splitpath_s(fname, drivebuf, 3, pathbuf, 1024, filebuf, 512, suffixbuf,
                     64) != 0) {
        return NULL;
    }
    size_t offset = strlen(drivebuf); /* includes room for colon */
    if ((offset > 0) && (strlen(pathbuf) > 0)) {
        memmove(pathbuf + offset, pathbuf, strlen(pathbuf));
        memcpy(pathbuf, drivebuf, offset);
    }
#else
    if (SysUtil::SplitPath(fname, pathbuf, 1024, filebuf, 512, suffixbuf, 64) !=
        0) {
        return NULL;
    }
#endif

    if (newSuffix && (strlen(newSuffix) > 0)) {
        size_t len = strlen(pathbuf) + strlen(filebuf) + strlen(newSuffix) + 3;
        newFname = (char *) malloc(len);
        if (strlen(pathbuf) == 0 ||
            strcmp(pathbuf, ".") ==
                0) { /* if it's "./foo", just make it "foo" */
            snprintf(newFname, len, "%s.%s", filebuf, newSuffix);
        } else {
            snprintf(newFname, len, "%s%c%s.%s", pathbuf, PATHSEP, filebuf,
                     newSuffix);
        }
    } else {
        size_t len = strlen(pathbuf) + strlen(filebuf) + 2;
        newFname = (char *) malloc(len);
        if (strlen(pathbuf) == 0 ||
            strcmp(pathbuf, ".") ==
                0) { /* if it's "./foo", just make it "foo" */
            snprintf(newFname, len, "%s", filebuf);
        } else {
            snprintf(newFname, len, "%s%c%s", pathbuf, PATHSEP, filebuf);
        }
    }
    return newFname;
}
