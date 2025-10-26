/** @file utilities.cpp
 *
 * File created by Peter Watkins (KE7IST) 8/12/16.
 * Derived from original project code.
 * Splat!
 * @copyright 1997 - 2018 John A. Magliacane (KD2BD) and contributors.
 * See revision control history for contributions.
 * This file is covered by the LICENSE.md file in the root of this project.
 */

#include "utilities.h"
#include "site.h"
#include "splat_run.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <math.h>
#include <sstream>
#include <string>

int Utilities::interpolate(int y0, int y1, int x0, int x1, int n) {
    /* Perform linear interpolation between quantized contour
     levels displayed in field strength and path loss maps.
     If signal level x0 corresponds to color level y0, signal
     level x1 corresponds to color level y1, and signal level
     n falls somewhere between x0 and x1, determine what
     color value n corresponds to between y0 and y1. */

    int result = 0;
    double delta_x, delta_y;

    if (n <= x0)
        return y0;

    if (n >= x1)
        return y1;

    if (y0 == y1)
        return y0;

    if (x0 == x1)
        return y0;

    delta_y = (double) (y0 - y1);
    delta_x = (double) (x0 - x1);

    result = y0 + (int) ceil((delta_y / delta_x) * (n - x0));

    return result;
}

double Utilities::arccos(double x, double y) {
    /* This function implements the arc cosine function,
     returning a value between 0 and TWOPI. */

    double result = 0.0;

    if (y > 0.0)
        result = acos(x / y);

    if (y < 0.0)
        result = PI + acos(x / y);

    return result;
}

int Utilities::ReduceAngle(double angle) {
    /* This function normalizes the argument to
     an integer angle between 0 and 180 degrees */

    double temp;

    temp = acos(cos(angle * DEG2RAD));

    return (int) rint(temp / DEG2RAD);
}

double Utilities::LonDiff(double lon1, double lon2) {
    /* This function returns the short path longitudinal
     difference between longitude1 and longitude2
     as an angle between -180.0 and +180.0 degrees.
     If lon1 is west of lon2, the result is positive.
     If lon1 is east of lon2, the result is negative. */

    double diff;

    diff = lon1 - lon2;

    if (diff <= -180.0)
        diff += 360.0;

    if (diff >= 180.0)
        diff -= 360.0;

    return diff;
}

std::string Utilities::dec2dms(double decimal) {
    /* Converts decimal degrees to degrees, minutes, seconds,
     (DMS) and returns the result as a character string. */

    char sign;
    int degrees, minutes, seconds;
    double a, b, c, d;

    if (decimal < 0.0) {
        decimal = -decimal;
        sign = -1;
    }

    else
        sign = 1;

    a = floor(decimal);
    b = 60.0 * (decimal - a);
    c = floor(b);
    d = 60.0 * (b - c);

    degrees = (int) a;
    minutes = (int) c;
    seconds = (int) d;

    if (seconds < 0)
        seconds = 0;

    if (seconds > 59)
        seconds = 59;

    std::ostringstream oss;
    oss << degrees * sign << "°" << ' ' << minutes << "\' " << seconds << '"';
    return oss.str();
}

double Utilities::ReadBearing(std::string_view input) {
    /* This function takes numeric input in the form of a character
     string, and returns an equivalent bearing in degrees as a
     decimal number (double).  The input may either be expressed
     in decimal format (40.139722) or degree, minute, second
     format (40 08 23).  This function also safely handles
     extra spaces found either leading, trailing, or
     embedded within the numbers expressed in the
     input string.  Decimal seconds are permitted. */

    double seconds, bearing = 0.0;
    char string[20];
    int a, b, degrees, minutes;

    /* Copy "input" to "string", and ignore any extra
     spaces that might be present in the process. */

    string[0] = 0;

    size_t length = input.size();
    for (a = 0, b = 0; a < (int) length && a < 18; a++) {
        if ((input[a] != 32 && input[a] != '\n') ||
            (input[a] == 32 && input[a + 1] != 32 && input[a + 1] != '\n' &&
             b != 0)) {
            string[b] = input[a];
            b++;
        }
    }

    string[b] = 0;

    /* Count number of spaces in the clean string. */

    length = strlen(string);

    for (a = 0, b = 0; a < (int) length; a++)
        if (string[a] == 32)
            b++;

    if (b == 0) /* Decimal Format (40.139722) */
        sscanf(string, "%lf", &bearing);

    if (b == 2) /* Degree, Minute, Second Format (40 08 23.xx) */
    {
        sscanf(string, "%d %d %lf", &degrees, &minutes, &seconds);

        bearing = fabs((double) degrees);
        bearing += fabs(((double) minutes) / 60.0);
        bearing += fabs(seconds / 3600.0);

        if ((degrees < 0) || (minutes < 0) || (seconds < 0.0))
            bearing = -bearing;
    }

    /* Anything else returns a 0.0 */

    if (bearing > 360.0 || bearing < -360.0)
        bearing = 0.0;

    return bearing;
}

std::string Utilities::PathLeaf(std::string_view path) {
    std::string::size_type idx;

    idx = path.rfind('/');

    // No delimeter found. Must just be a filename. Return that.
    if (idx == std::string::npos) {
        return std::string(path);
    }

    return std::string(path.substr(idx + 1));
}

std::string::size_type Utilities::ExtensionIdx(std::string_view path) {
    std::string::size_type idx;

    std::string leaf = Utilities::PathLeaf(path);

    idx = leaf.rfind('.');

    // No delimeter found. There must be no extension specified. Use the
    // default.
    if (idx == std::string::npos) {
        return idx;
    }

    // We know idx into leaf.
    return path.size() - leaf.size() + idx;
}

std::string Utilities::Basename(std::string_view path) {
    std::string::size_type idx = Utilities::ExtensionIdx(path);
    return idx == std::string::npos ? std::string(path)
                                    : std::string(path.substr(0, idx));
}

std::string Utilities::Extension(std::string_view path) {
    std::string::size_type idx = Utilities::ExtensionIdx(path);
    return idx == std::string::npos ? "" : std::string(path.substr(idx + 1));
}

std::string Utilities::DivideExtension(std::string &path,
                                       const std::string &default_extension) {
    std::string::size_type idx = Utilities::ExtensionIdx(path);

    // No delimeter found. There must be no extension specified. Use the
    // default.
    if (idx == std::string::npos) {
        return default_extension;
    }

    // We know idx into leaf. Return just the path minus the extension.
    std::string ext = path.substr(idx + 1);
    path = path.substr(0, idx);
    return ext;
}

void Utilities::Chomp(std::string &str) {
    str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());
    str.erase(std::remove(str.begin(), str.end(), '\r'), str.end());
}
