/** @file image.cpp
 *
 * File created by Peter Watkins (KE7IST) 1/8/18.
 * Derived from original project code.
 * Splat!
 * @copyright 1997 - 2018 John A. Magliacane (KD2BD) and contributors.
 * See revision control history for contributions.
 * This file is covered by the LICENSE.md file in the root of this project.
 */

#include "image.h"
#include "dem.h"
#include "elevation_map.h"
#include "fontdata.h"
#include "lrp.h"
#include "antenna_pattern.h"
#include "path.h"
#include "region.h"
#include "sdf.h"
#include "site.h"
#include "splat_run.h"
#include "utilities.h"
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>


#ifndef _WIN32
#define RGB(signal, r, g, b) (((uint32_t)(uint8_t)signal) | ((uint32_t)((uint8_t)r) << 8) | ((uint32_t)((uint8_t)g) << 16) | ((uint32_t)((uint8_t)b) << 24))
#endif

#define COLOR_RED(s) (RGB(s, 255, 0, 0))
#define COLOR_LIGHTCYAN(s) (RGB(s, 128, 128, 255))
#define COLOR_GREEN(s) (RGB(s, 0, 255, 0))
#define COLOR_DARKGREEN(s) (RGB(s, 0, 100, 0))
#define COLOR_DARKSEAGREEN1(s) (RGB(s, 193, 255, 193))
#define COLOR_CYAN(s) (RGB(s, 0, 255, 255))
#define COLOR_YELLOW(s) (RGB(s, 255, 255, 0))
#define COLOR_GREENYELLOW(s) (RGB(s, 173, 255, 47))
#define COLOR_MEDIUMSPRINGGREEN(s) (RGB(s, 0, 250, 154))
#define COLOR_MEDIUMVIOLET(s) (RGB(s, 147, 112, 219))
#define COLOR_PINK(s) (RGB(s, 255, 192, 203))
#define COLOR_ORANGE(s) (RGB(s, 255, 165, 0))
#define COLOR_SIENNA(s) (RGB(s, 255, 130, 71))
#define COLOR_BLANCHEDALMOND(s) (RGB(s, 255, 235, 205))
#define COLOR_DARKTURQUOISE(s) (RGB(s, 0, 206, 209))
#define COLOR_TAN(s) (RGB(s, 210, 180, 140))
#define COLOR_GOLD2(s) (RGB(s, 238, 201, 0))
#define COLOR_MEDIUMBLUE(s) (RGB(s, 0, 0, 170))
#define COLOR_WHITE(s) (RGB(s, 255, 255, 255))
#define COLOR_BLACK(s) (RGB(s, 0, 0, 0))

Image::Image(const SplatRun &sr, std::string &filename,
      const std::vector<Site> &xmtr, const ElevationMap &em)
    :
sr(sr),
em(em),
filename(filename),
xmtr(xmtr),
conversion(255.0 / pow((double)(em.max_elevation - em.min_elevation), one_over_gamma))
{}

Pixel Image::GetPixel(const Dem *dem, MapType maptype, Region &region, int x0, int y0) {

    unsigned int red, green, blue, terrain;

    if (dem == NULL) {
        return COLOR_BLACK(255);
    }

    unsigned char mask = dem->mask[x0 * sr.ippd + y0];

    /* Note: The array dem->signal holds a scaled value depending
     * on the type that is unscaled again in the following.
     * See function PlotLRPath() in elevation_map.cpp
     */
    int pathloss=255;
    int signal=255;

    pathloss = (dem->signal[x0 * sr.ippd + y0]);

    if(maptype == MAPTYPE_DBM) {
        // signal contains the power level in dBm
        signal = pathloss - 200;
    } else if(maptype == MAPTYPE_DBUVM) {
        // signal contains the power level in dBuV/m
        signal = pathloss - 100;
    } else if(maptype == MAPTYPE_PATHLOSS) {
        // signal contains the path loss in dB
        signal = pathloss;
    }

    int match = 255;

    red = 0;
    green = 0;
    blue = 0;

    if(maptype != MAPTYPE_PATHLOSS) {
        // for dBm and dBuV/m output
        if (signal >= region.level[0]) {
            match = 0;
        } else {
            for (int z = 1; (z < region.levels && match == 255); z++) {
                if (signal < region.level[z - 1] && signal >= region.level[z]) {
                    match = z;
                }
            }
        }

        if (match < region.levels) {
            if (sr.smooth_contours && match > 0) {
                red = (unsigned)Utilities::interpolate(
                                                       region.color[match][0],
                                                       region.color[match - 1][0], region.level[match],
                                                       region.level[match - 1], signal);
                green = (unsigned)Utilities::interpolate(
                                                         region.color[match][1],
                                                         region.color[match - 1][1], region.level[match],
                                                         region.level[match - 1], signal);
                blue = (unsigned)Utilities::interpolate(
                                                        region.color[match][2],
                                                        region.color[match - 1][2], region.level[match],
                                                        region.level[match - 1], signal);
            } else {
                red = region.color[match][0];
                green = region.color[match][1];
                blue = region.color[match][2];
            }
        }

    } else {
        // PathLoss is calculated differently from dBm and dBuV/m
        if (signal <= region.level[0]) {
            match = 0;
        } else {
            for (int z = 1; (z < region.levels && match == 255); z++) {
                if (signal >= region.level[z - 1] && signal < region.level[z]) {
                    match = z;
                }
            }
        }

        if (match < region.levels) {
            if (sr.smooth_contours && match > 0) {
                red = (unsigned)Utilities::interpolate(
                                                       region.color[match - 1][0],
                                                       region.color[match][0], region.level[match - 1],
                                                       region.level[match], signal);
                green = (unsigned)Utilities::interpolate(
                                                         region.color[match - 1][1],
                                                         region.color[match][1], region.level[match - 1],
                                                         region.level[match], signal);
                blue = (unsigned)Utilities::interpolate(
                                                        region.color[match - 1][2],
                                                        region.color[match][2], region.level[match - 1],
                                                        region.level[match], signal);
            } else {
                red = region.color[match][0];
                green = region.color[match][1];
                blue = region.color[match][2];
            }
        }
    }

    Pixel pixel;

    if (mask & 2) {
        /* Text Labels: Red or otherwise */

        if (red >= 180 && green <= 75 && blue <= 75 && signal != 0) {
            pixel = RGB(pathloss, 255 ^ red, 255 ^ green, 255 ^ blue);
        } else {
            pixel = COLOR_RED(pathloss);
        }
    } else if (mask & 4) {
        /* County Boundaries: Black */
        pixel = COLOR_BLACK(pathloss);
    } else {
        switch (maptype)
        {
            case MAPTYPE_PATHLOSS:
                // different for PathLoss
                  if (signal == 0 || (sr.contour_threshold != 0 && signal > abs(sr.contour_threshold))) {
                      if (sr.ngs) {
                          /* No terrain */
                          pixel = COLOR_WHITE(pathloss);
                      } else {
                          /* Display land or sea elevation */
                          if (dem->data[x0 * sr.ippd + y0] == 0) {
                              pixel = COLOR_MEDIUMBLUE(pathloss);
                          } else {
                              terrain = (unsigned)(0.5 + pow((double)(dem->data[x0 * sr.ippd + y0] - em.min_elevation), one_over_gamma) * conversion);
                              pixel = RGB(pathloss, terrain, terrain, terrain);
                          }
                      }
                  } else {
                      /* Plot signal level regions in color */
                      if (red != 0 || green != 0 || blue != 0) {
                          pixel = RGB(pathloss, red, green, blue);
                      } else { /* terrain / sea-level */
                          if (dem->data[x0 * sr.ippd + y0] == 0) {
                              pixel = COLOR_MEDIUMBLUE(pathloss);
                          } else {
                              /* Elevation: Greyscale */
                              terrain = (unsigned)(0.5 + pow((double)(dem->data[x0 * sr.ippd + y0] - em.min_elevation), one_over_gamma) * conversion);
                              pixel = RGB(pathloss, terrain, terrain, terrain);
                          }
                      }
                  }
                break;
            case MAPTYPE_LOS:
                 switch (mask & 57) {
                     case 1:
                         /* TX1: Green */
                         pixel = COLOR_GREEN(pathloss);
                         break;

                     case 8:
                         /* TX2: Cyan */
                         pixel = COLOR_CYAN(pathloss);
                         break;

                     case 9:
                         /* TX1 + TX2: Yellow */
                         pixel = COLOR_YELLOW(pathloss);
                         break;

                     case 16:
                         /* TX3: Medium Violet */
                         pixel = COLOR_MEDIUMVIOLET(pathloss);
                         break;

                     case 17:
                         /* TX1 + TX3: Pink */
                         pixel = COLOR_PINK(pathloss);
                         break;

                     case 24:
                         /* TX2 + TX3: Orange */
                         pixel = COLOR_ORANGE(pathloss);
                         break;

                     case 25:
                         /* TX1 + TX2 + TX3: Dark Green */
                         pixel = COLOR_DARKGREEN(pathloss);
                         break;

                     case 32:
                         /* TX4: Sienna 1 */
                         pixel = COLOR_SIENNA(pathloss);
                         break;

                     case 33:
                         /* TX1 + TX4: Green Yellow */
                         pixel = COLOR_GREENYELLOW(pathloss);
                         break;

                     case 40:
                         /* TX2 + TX4: Dark Sea Green 1 */
                         pixel = COLOR_DARKSEAGREEN1(pathloss);
                         break;

                     case 41:
                         /* TX1 + TX2 + TX4: Blanched Almond */
                         pixel = COLOR_BLANCHEDALMOND(pathloss);
                         break;

                     case 48:
                         /* TX3 + TX4: Dark Turquoise */
                         pixel = COLOR_DARKTURQUOISE(pathloss);
                         break;

                     case 49:
                         /* TX1 + TX3 + TX4: Medium Spring Green */
                         pixel = COLOR_MEDIUMSPRINGGREEN(pathloss);
                         break;

                     case 56:
                         /* TX2 + TX3 + TX4: Tan */
                         pixel = COLOR_TAN(pathloss);
                         break;

                     case 57:
                         /* TX1 + TX2 + TX3 + TX4: Gold2 */
                         pixel = COLOR_GOLD2(pathloss);
                         break;

                     default:
                         if (sr.ngs) /* No terrain */
                             pixel = COLOR_MEDIUMBLUE(pathloss);
                         else {
                             /* Sea-level: Medium Blue */
                             if (dem->data[x0 * sr.ippd + y0] == 0)
                                 pixel = COLOR_MEDIUMBLUE(pathloss);
                             else {
                                 /* Elevation: Greyscale */
                                 terrain = (unsigned)(0.5 + pow((double)(dem->data[x0 * sr.ippd + y0] - em.min_elevation), one_over_gamma) * conversion);
                                 pixel = RGB(pathloss, terrain, terrain, terrain);
                             }
                         }
                     }
                break;
            default:
                if (sr.contour_threshold != 0 && signal < sr.contour_threshold) {
                    if (sr.ngs) {
                        /* No terrain */
                        pixel = COLOR_WHITE(pathloss);
                    } else {
                        /* Display land or sea elevation */
                        if (dem->data[x0 * sr.ippd + y0] == 0) {
                            pixel = COLOR_MEDIUMBLUE(pathloss);
                        } else {
                            terrain = (unsigned)(0.5 + pow((double)(dem->data[x0 * sr.ippd + y0] - em.min_elevation), one_over_gamma) * conversion);
                            pixel = RGB(pathloss, terrain, terrain, terrain);
                        }
                    }
                } else {
                    /* Plot signal level regions in color */
                    if (red != 0 || green != 0 || blue != 0) {
                        pixel = RGB(pathloss, red, green, blue);
                    }
                    else /* terrain / sea-level */
                    {
                        if (sr.ngs) {
                            pixel = COLOR_WHITE(pathloss);
                        } else {
                            if (dem->data[x0 * sr.ippd + y0] == 0) {
                                pixel = COLOR_MEDIUMBLUE(pathloss);
                            } else {
                                /* Elevation: Greyscale */
                                terrain = (unsigned)(0.5 + pow((double)(dem->data[x0 * sr.ippd + y0] - em.min_elevation), one_over_gamma) * conversion);
                                pixel = RGB(pathloss, terrain, terrain, terrain);
                            }
                        }
                    }
                }
                // We assume the map type is MAPTYPE_DBM or MAPTYPE_DBUVM
                break;
        }
    }
    return pixel;
}

int Image::GetIndexForColorKeyImageFile(MapType maptype, Region &region, int x0, int y0) {
    int level, hundreds, tens, units;
    int indx = y0 / 30;
    int x = x0;
    
    if(maptype == MAPTYPE_DBM) {
        level = abs(region.level[indx]);
        
        hundreds = level / 100;
        
        if (hundreds > 0)
            level -= (hundreds * 100);
        
        tens = level / 10;
        
        if (tens > 0)
            level -= (tens * 10);
        
        units = level;
        
        if ((y0 % 30) >= 8 && (y0 % 30) <= 23) {
            if (hundreds > 0) {
                if (region.level[indx] < 0) {
                    if (x >= 5 && x <= 12)
                        if (fontdata[16 * ('-') + ((y0 % 30) - 8)] &
                            (128 >> (x - 5)))
                            indx = 255;
                }
                
                else {
                    if (x >= 5 && x <= 12)
                        if (fontdata[16 * ('+') + ((y0 % 30) - 8)] &
                            (128 >> (x - 5)))
                            indx = 255;
                }
                
                if (x >= 13 && x <= 20)
                    if (fontdata[16 * (hundreds + '0') +
                                 ((y0 % 30) - 8)] &
                        (128 >> (x - 13)))
                        indx = 255;
            }
            
            if (tens > 0 || hundreds > 0) {
                if (hundreds == 0) {
                    if (region.level[indx] < 0) {
                        if (x >= 13 && x <= 20)
                            if (fontdata[16 * ('-') +
                                         ((y0 % 30) - 8)] &
                                (128 >> (x - 13)))
                                indx = 255;
                    }
                    
                    else {
                        if (x >= 13 && x <= 20)
                            if (fontdata[16 * ('+') +
                                         ((y0 % 30) - 8)] &
                                (128 >> (x - 13)))
                                indx = 255;
                    }
                }
                
                if (x >= 21 && x <= 28)
                    if (fontdata[16 * (tens + '0') +
                                 ((y0 % 30) - 8)] &
                        (128 >> (x - 21)))
                        indx = 255;
            }
            
            if (hundreds == 0 && tens == 0) {
                if (region.level[indx] < 0) {
                    if (x >= 21 && x <= 28)
                        if (fontdata[16 * ('-') + ((y0 % 30) - 8)] &
                            (128 >> (x - 21)))
                            indx = 255;
                }
                
                else {
                    if (x >= 21 && x <= 28)
                        if (fontdata[16 * ('+') + ((y0 % 30) - 8)] &
                            (128 >> (x - 21)))
                            indx = 255;
                }
            }
            
            if (x >= 29 && x <= 36)
                if (fontdata[16 * (units + '0') + ((y0 % 30) - 8)] &
                    (128 >> (x - 29)))
                    indx = 255;
            
            if (x >= 37 && x <= 44)
                if (fontdata[16 * ('d') + ((y0 % 30) - 8)] &
                    (128 >> (x - 37)))
                    indx = 255;
            
            if (x >= 45 && x <= 52)
                if (fontdata[16 * ('B') + ((y0 % 30) - 8)] &
                    (128 >> (x - 45)))
                    indx = 255;
            
            if (x >= 53 && x <= 60)
                if (fontdata[16 * ('m') + ((y0 % 30) - 8)] &
                    (128 >> (x - 53)))
                    indx = 255;
        }
    } // end dBm
    
    else if(maptype == MAPTYPE_DBUVM) {
        level = region.level[indx];
        
        hundreds = level / 100;
        
        if (hundreds > 0)
            level -= (hundreds * 100);
        
        tens = level / 10;
        
        if (tens > 0)
            level -= (tens * 10);
        
        units = level;
        
        if ((y0 % 30) >= 8 && (y0 % 30) <= 23) {
            if (hundreds > 0) {
                if (x >= 5 && x <= 12)
                    if (fontdata[16 * (hundreds + '0') +
                                 ((y0 % 30) - 8)] &
                        (128 >> (x - 5)))
                        indx = 255;
            }
            
            if (tens > 0 || hundreds > 0) {
                if (x >= 13 && x <= 20)
                    if (fontdata[16 * (tens + '0') +
                                 ((y0 % 30) - 8)] &
                        (128 >> (x - 13)))
                        indx = 255;
            }
            
            if (x >= 21 && x <= 28)
                if (fontdata[16 * (units + '0') + ((y0 % 30) - 8)] &
                    (128 >> (x - 21)))
                    indx = 255;
            
            if (x >= 36 && x <= 43)
                if (fontdata[16 * ('d') + ((y0 % 30) - 8)] &
                    (128 >> (x - 36)))
                    indx = 255;
            
            if (x >= 44 && x <= 51)
                if (fontdata[16 * ('B') + ((y0 % 30) - 8)] &
                    (128 >> (x - 44)))
                    indx = 255;
            
            if (x >= 52 && x <= 59)
                if (fontdata[16 * (230) + ((y0 % 30) - 8)] &
                    (128 >> (x - 52)))
                    indx = 255;
            
            if (x >= 60 && x <= 67)
                if (fontdata[16 * ('V') + ((y0 % 30) - 8)] &
                    (128 >> (x - 60)))
                    indx = 255;
            
            if (x >= 68 && x <= 75)
                if (fontdata[16 * ('/') + ((y0 % 30) - 8)] &
                    (128 >> (x - 68)))
                    indx = 255;
            
            if (x >= 76 && x <= 83)
                if (fontdata[16 * ('m') + ((y0 % 30) - 8)] &
                    (128 >> (x - 76)))
                    indx = 255;
        }
    } // end dBuV/m
    
    else if (maptype == MAPTYPE_PATHLOSS) {
        level = region.level[indx];
        
        hundreds = level / 100;
        
        if (hundreds > 0)
            level -= (hundreds * 100);
        
        tens = level / 10;
        
        if (tens > 0)
            level -= (tens * 10);
        
        units = level;
        
        if ((y0 % 30) >= 8 && (y0 % 30) <= 23) {
            if (hundreds > 0) {
                if (x >= 11 && x <= 18)
                    if (fontdata[16 * (hundreds + '0') +
                                 ((y0 % 30) - 8)] &
                        (128 >> (x - 11)))
                        indx = 255;
            }
            
            if (tens > 0 || hundreds > 0) {
                if (x >= 19 && x <= 26)
                    if (fontdata[16 * (tens + '0') +
                                 ((y0 % 30) - 8)] &
                        (128 >> (x - 19)))
                        indx = 255;
            }
            
            if (x >= 27 && x <= 34)
                if (fontdata[16 * (units + '0') + ((y0 % 30) - 8)] &
                    (128 >> (x - 27)))
                    indx = 255;
            
            if (x >= 42 && x <= 49)
                if (fontdata[16 * ('d') + ((y0 % 30) - 8)] &
                    (128 >> (x - 42)))
                    indx = 255;
            
            if (x >= 50 && x <= 57)
                if (fontdata[16 * ('B') + ((y0 % 30) - 8)] &
                    (128 >> (x - 50)))
                    indx = 255;
        }
    }
    
    return indx;
}

void Image::WriteColorKeyImageFile(const std::string &ckfile, ImageType imagetype, MapType maptype, Region &region) {
    unsigned int height = 30 * region.levels;
    unsigned int width = 100;
    
    try {
        ImageWriter iw = ImageWriter(ckfile, imagetype, width, height,0,0,0,0);
        
        for (int y0 = 0; y0 < (int)height; y0++) {
            for (int x0 = 0; x0 < (int)width; x0++) {
                int indx = GetIndexForColorKeyImageFile(maptype, region, x0, y0); // end pathloss

                Pixel pixel;
                if (indx > region.levels) {
                    pixel = COLOR_BLACK(0);
                } else {
                    unsigned int red = region.color[indx][0];
                    unsigned int green = region.color[indx][1];
                    unsigned int blue = region.color[indx][2];

                    pixel = RGB(0, red, green, blue);
                }

                iw.AppendPixel(pixel);
            }

            iw.EmitLine();
        }

        iw.Finish();
    } catch (const std::exception &e) {
        std::cerr << "Error writing " << ckfile << ": " << e.what()
                  << std::endl;
        return;
    }
}

int Image::GetIndexForLegend(int colorwidth, MapType maptype, Region &region, int x0, int y0) {
    int level, hundreds, tens, units;
    int indx = x0 / colorwidth;
    int x = x0 % colorwidth;
    
    if(maptype == MAPTYPE_DBM) {
        level = abs(region.level[indx]);
        
        hundreds = level / 100;
        
        if (hundreds > 0)
            level -= (hundreds * 100);
        
        tens = level / 10;
        
        if (tens > 0)
            level -= (tens * 10);
        
        units = level;
        
        if (y0 >= 8 && y0 <= 23) {
            if (hundreds > 0) {
                if (region.level[indx] < 0) {
                    if (x >= 5 && x <= 12)
                        if (fontdata[16 * ('-') + (y0 - 8)] &
                            (128 >> (x - 5)))
                            indx = 255;
                }
                
                else {
                    if (x >= 5 && x <= 12)
                        if (fontdata[16 * ('+') + (y0 - 8)] &
                            (128 >> (x - 5)))
                            indx = 255;
                }
                
                if (x >= 13 && x <= 20)
                    if (fontdata[16 * (hundreds + '0') + (y0 - 8)] &
                        (128 >> (x - 13)))
                        indx = 255;
            }
            
            if (tens > 0 || hundreds > 0) {
                if (hundreds == 0) {
                    if (region.level[indx] < 0) {
                        if (x >= 13 && x <= 20)
                            if (fontdata[16 * ('-') + (y0 - 8)] &
                                (128 >> (x - 13)))
                                indx = 255;
                    }
                    
                    else {
                        if (x >= 13 && x <= 20)
                            if (fontdata[16 * ('+') + (y0 - 8)] &
                                (128 >> (x - 13)))
                                indx = 255;
                    }
                }
                
                if (x >= 21 && x <= 28)
                    if (fontdata[16 * (tens + '0') + (y0 - 8)] &
                        (128 >> (x - 21)))
                        indx = 255;
            }
            
            if (hundreds == 0 && tens == 0) {
                if (region.level[indx] < 0) {
                    if (x >= 21 && x <= 28)
                        if (fontdata[16 * ('-') + (y0 - 8)] &
                            (128 >> (x - 21)))
                            indx = 255;
                }
                
                else {
                    if (x >= 21 && x <= 28)
                        if (fontdata[16 * ('+') + (y0 - 8)] &
                            (128 >> (x - 21)))
                            indx = 255;
                }
            }
            
            if (x >= 29 && x <= 36)
                if (fontdata[16 * (units + '0') + (y0 - 8)] &
                    (128 >> (x - 29)))
                    indx = 255;
            
            if (x >= 37 && x <= 44)
                if (fontdata[16 * ('d') + (y0 - 8)] &
                    (128 >> (x - 37)))
                    indx = 255;
            
            if (x >= 45 && x <= 52)
                if (fontdata[16 * ('B') + (y0 - 8)] &
                    (128 >> (x - 45)))
                    indx = 255;
            
            if (x >= 53 && x <= 60)
                if (fontdata[16 * ('m') + (y0 - 8)] &
                    (128 >> (x - 53)))
                    indx = 255;
        }
    } // end dBm
    
    else if (maptype == MAPTYPE_DBUVM) {
        level = region.level[indx];
        
        hundreds = level / 100;
        
        if (hundreds > 0)
            level -= (hundreds * 100);
        
        tens = level / 10;
        
        if (tens > 0)
            level -= (tens * 10);
        
        units = level;
        
        
        if (y0 >= 8 && y0 <= 23) {
            if (hundreds > 0) {
                if (x >= 5 && x <= 12)
                    if (fontdata[16 * (hundreds + '0') + (y0 - 8)] &
                        (128 >> (x - 5)))
                        indx = 255;
            }
            
            if (tens > 0 || hundreds > 0) {
                if (x >= 13 && x <= 20)
                    if (fontdata[16 * (tens + '0') + (y0 - 8)] &
                        (128 >> (x - 13)))
                        indx = 255;
            }
            
            if (x >= 21 && x <= 28)
                if (fontdata[16 * (units + '0') + (y0 - 8)] &
                    (128 >> (x - 21)))
                    indx = 255;
            
            if (x >= 36 && x <= 43)
                if (fontdata[16 * ('d') + (y0 - 8)] &
                    (128 >> (x - 36)))
                    indx = 255;
            
            if (x >= 44 && x <= 51)
                if (fontdata[16 * ('B') + (y0 - 8)] &
                    (128 >> (x - 44)))
                    indx = 255;
            
            if (x >= 52 && x <= 59)
                if (fontdata[16 * (230) + (y0 - 8)] &
                    (128 >> (x - 52)))
                    indx = 255;
            
            if (x >= 60 && x <= 67)
                if (fontdata[16 * ('V') + (y0 - 8)] &
                    (128 >> (x - 60)))
                    indx = 255;
            
            if (x >= 68 && x <= 75)
                if (fontdata[16 * ('/') + (y0 - 8)] &
                    (128 >> (x - 68)))
                    indx = 255;
            
            if (x >= 76 && x <= 83)
                if (fontdata[16 * ('m') + (y0 - 8)] &
                    (128 >> (x - 76)))
                    indx = 255;
        }
    } // end dBuV/m
    
    else if(maptype == MAPTYPE_PATHLOSS) {
        level = region.level[indx];
        
        hundreds = level / 100;
        
        if (hundreds > 0)
            level -= (hundreds * 100);
        
        tens = level / 10;
        
        if (tens > 0)
            level -= (tens * 10);
        
        units = level;
        
        if (y0 >= 8 && y0 <= 23) {
            if (hundreds > 0) {
                if (x >= 11 && x <= 18)
                    if (fontdata[16 * (hundreds + '0') + (y0 - 8)] &
                        (128 >> (x - 11)))
                        indx = 255;
            }
            
            if (tens > 0 || hundreds > 0) {
                if (x >= 19 && x <= 26)
                    if (fontdata[16 * (tens + '0') + (y0 - 8)] &
                        (128 >> (x - 19)))
                        indx = 255;
            }
            
            if (x >= 27 && x <= 34)
                if (fontdata[16 * (units + '0') + (y0 - 8)] &
                    (128 >> (x - 27)))
                    indx = 255;
            
            if (x >= 42 && x <= 49)
                if (fontdata[16 * ('d') + (y0 - 8)] &
                    (128 >> (x - 42)))
                    indx = 255;
            
            if (x >= 50 && x <= 57)
                if (fontdata[16 * ('B') + (y0 - 8)] &
                    (128 >> (x - 50)))
                    indx = 255;
        }
    }
    
    return indx;
}

void Image::WriteLegend(ImageWriter &iw, MapType maptype, Region &region, unsigned int width) {
    int colorwidth = (int)rint((float)width / (float)region.levels);
    
    for (int y0 = 0; y0 < 30; y0++) {
        for (int x0 = 0; x0 < (int)width; x0++) {
            int indx = GetIndexForLegend(colorwidth, maptype, region, x0, y0);

            Pixel pixel;
            if (indx > region.levels) {
                pixel = COLOR_BLACK(0);
            } else {
                unsigned int red = region.color[indx][0];
                unsigned int green = region.color[indx][1];
                unsigned int blue = region.color[indx][2];

                pixel = RGB(0, red, green, blue);
            }

            iw.AppendPixel(pixel);
        }
        iw.EmitLine();
    }
}

/* Generates a coverage map based on the calculated values held in the
 * signal[][] array. The image created is rotated counter-clockwise 90 degrees
 * from its representation in em.dem[][] so that north points up and east points
 * right.
 *
 * Select which map type you want to write:
 * - power level in dBm
 * - electric field strength in dBuV/m
 * - path loss in dB
 * 
 */
 
void Image::WriteCoverageMap(MapType maptype, ImageType imagetype, Region &region) {
    std::string basename, mapfile, geofile, kmlfile, ckfile, suffix, description;
#if DO_KMZ
    std::string kmzfile;
#endif
    unsigned int width, height;
    unsigned int imgheight, imgwidth;
    double lat, lon, north, south, east, west, minwest;
    FILE *fd;

    width = (unsigned)(sr.ippd * Utilities::ReduceAngle(em.max_west - em.min_west));
    height = (unsigned)(sr.ippd * Utilities::ReduceAngle(em.max_north - em.min_north));

	switch (maptype) {
	case MAPTYPE_DBM:
		region.LoadDBMColors(xmtr[0]);
		description = "Power Level (dBm)";
		break;
	case MAPTYPE_DBUVM:
		region.LoadSignalColors(xmtr[0]);
		description = "Electric Field Strength (dBuV/m)";
		break;
	case MAPTYPE_PATHLOSS:
		region.LoadLossColors(xmtr[0]);
		description = "Path Loss (dB)";
		break;
	case MAPTYPE_LOS:
		description = "Line of Sight";
        // PVW: TODO remove comment
		//std::cerr << "Not yet implemented! Use Image::WriteImage() instead.";
		//exit(1);
		break;
	}

    switch (imagetype) {
    default:
#ifdef HAVE_LIBPNG
    case IMAGETYPE_PNG:
        suffix = ".png";
        break;
#endif
#ifdef HAVE_LIBGDAL
    case IMAGETYPE_GEOTIFF:
        suffix = ".tif";
        break;
#endif
#ifdef HAVE_LIBJPEG
    case IMAGETYPE_JPG:
        suffix = ".jpg";
        break;
#endif
    case IMAGETYPE_PPM:
        suffix = ".ppm";
        break;
    }

    if (filename.empty()) {
        basename = Utilities::Basename(xmtr[0].filename);
        filename = basename + suffix;
    } else {
        basename = Utilities::Basename(filename);
    }

    mapfile = basename + suffix;
    geofile = basename + ".geo";
    kmlfile = basename + ".kml";
#if DO_KMZ
    kmzfile = basename + ".kmz";
#endif
    ckfile = basename + "-ck" + suffix;

    minwest = ((double)em.min_west) + sr.dpp;

    if (minwest > 360.0)
        minwest -= 360.0;

    north = (double)em.max_north - sr.dpp;

    if (sr.bottom_legend == false) {
		/* No bottom legend */
        south = (double)em.min_north;
        imgwidth = width;
        imgheight = height;
    } else {
		/* 30 pixels for bottom legend */
        south = (double)em.min_north - (30.0 / sr.ppd);
        imgwidth = width;
        imgheight = height + 30;
    }

    east = (minwest < 180.0 ? -minwest : 360.0 - em.min_west);
    west = (double)(em.max_west < 180 ? -em.max_west : 360 - em.max_west);

    if (sr.geo) {
        WriteGeo(geofile, mapfile, north, south, east, west, width, height);
    }

    // TODO: PVW: Why can't we write both KML and Geo?
    if (sr.kml && (sr.geo == 0)) {
        WriteKmlForImage("SPLAT! " + description + " Contours",
                         xmtr[0].name + " Transmitter Contours", sr.bottom_legend, kmlfile,
                         mapfile, north, south, east, west, ckfile);
    }

    fd = fopen(mapfile.c_str(), "wb");

	std::cout << "Writing " << description << " map \"" << mapfile << "\" (" << imgwidth << "x" << imgheight << " image)...\n";

    try {
        ImageWriter iw = ImageWriter(mapfile, imagetype, imgwidth, imgheight, north, south, east, west);
        int y;
        for (y = 0, lat = north; y < (int)height; y++, lat = north - (sr.dpp * (double)y)) {
            int x;
            for (x = 0, lon = em.max_west; x < (int)width; x++, lon = em.max_west - (sr.dpp * (double)x)) {
                if (lon < 0.0)
                    lon += 360.0;

                int x0 = 0, y0 = 0;
                const Dem *dem = em.FindDEM(lat, lon, x0, y0);
                Pixel pixel = GetPixel(dem, maptype, region, x0, y0);

                iw.AppendPixel(pixel);
            }

            iw.EmitLine();
        }

        if (sr.bottom_legend) {
            /* Display legend along bottom of image
             if not generating .sr.kml or .geo output. */
            WriteLegend(iw, maptype, region, width);
        }

        iw.Finish();
    } catch (const std::exception &e) {
        std::cerr << "Error writing " << mapfile << ": " << e.what()
                  << std::endl;
        return;
    }

    if (sr.kml) {
        /* Write colorkey image file */
        WriteColorKeyImageFile(ckfile, imagetype, maptype, region);
        
#if DO_KMZ
        bool success = false;
        struct zip_t *zip =
            zip_open(kmzfile, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');
        if (zip) {
            /* Pack the KML */
            if (zip_entry_open(zip, kmlfile) == 0) {
                if (zip_entry_fwrite(zip, kmlfile) == 0) {
                    success = true;
                }
                zip_entry_close(zip);
            }
            /* Pack the -ck file */
            if (zip_entry_open(zip, ckfile) == 0) {
                if (zip_entry_fwrite(zip, ckfile) == 0) {
                    success = true;
                }
                zip_entry_close(zip);
            }
            /* Pack the map image */
            if (zip_entry_open(zip, mapfile) == 0) {
                if (zip_entry_fwrite(zip, mapfile) == 0) {
                    success = true;
                }
                zip_entry_close(zip);
            }
            zip_close(zip);
        }

        if (success) {
            unlink(mapfile);
            unlink(kmlfile);
            unlink(ckfile);
            fprintf(stdout, "\nKMZ file written to: \"%s\"\n", kmzfile);
        } else {
            unlink(kmzfile);
            fprintf(stdout, "\nCouldn't create KMZ file.\n");
        }
#endif
    }

    fprintf(stdout, "Done!\n");
    fflush(stdout);
}

void Image::WriteKmlForImage(const std::string &groundOverlayName,
                             const std::string &description, bool writeScreenOverlay,
                             const std::string &kmlfile, const std::string &mapfile,
                             double north, double south, double east,
                             double west, const std::string &ckfile) {
    std::fstream fs;
    fs.open(kmlfile.c_str(), std::fstream::out);

    // TODO: Should we fail silently if we can't open. Shouldn't we WARN?
    if (!fs) {
        return;
    }

    // Use 5 decimal places.
    std::ios::fmtflags fsOriginalFlags(fs.flags());
    fs.setf(std::ios::fixed, std::ios::floatfield);
    fs.precision(5);

    fs << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
          "<kml xmlns=\"http://earth.google.com/kml/2.1\">\n"
          "<!-- Generated by "
       << SplatRun::splat_name << " Version " << SplatRun::splat_version
       << " -->\n"
          "  <Folder>\n"
          "   <name>"
       << SplatRun::splat_name
       << "</name>\n"
          "     <description>"
       << xmtr[0].name
       << " Transmitter Contours</description>\n"
          "       <GroundOverlay>\n"
          "         <name>"
       << groundOverlayName
       << "</name>\n"
          "           <description>"
       << description
       << "</description>\n"
          "		<Icon>\n"
          "              <href>"
       << mapfile
       << "</href>\n"
          "		</Icon>\n"
          /*       "            <opacity>128</opacity>\n" */
          "            <LatLonBox>\n"
          "               <north>"
       << north
       << "</north>\n"
          "               <south>"
       << south
       << "</south>\n"
          "               <east>"
       << east
       << "</east>\n"
          "               <west>"
       << west
       << "</west>\n"
          "               <rotation>0.0</rotation>\n"
          "            </LatLonBox>\n"
          "       </GroundOverlay>\n";
    if (writeScreenOverlay) {
        fs << "       <ScreenOverlay>\n"
              "          <name>Color Key</name>\n"
              "            <description>Contour Color Key</description>\n"
              "          <Icon>\n"
              "            <href>"
           << ckfile
           << "</href>\n"
              "          </Icon>\n"
              "          <overlayXY x=\"0\" y=\"1\" xunits=\"fraction\" "
              "yunits=\"fraction\"/>\n"
              "          <screenXY x=\"0\" y=\"1\" xunits=\"fraction\" "
              "yunits=\"fraction\"/>\n"
              "          <rotationXY x=\"0\" y=\"0\" xunits=\"fraction\" "
              "yunits=\"fraction\"/>\n"
              "          <size x=\"0\" y=\"0\" xunits=\"fraction\" "
              "yunits=\"fraction\"/>\n"
              "       </ScreenOverlay>\n";
    }

    // Restore the floating point defaults. TODO: Why use different precision on
    // these floating point numbers?
    fs.flags(fsOriginalFlags);

    for (int x = 0; x < (int)xmtr.size(); x++) {
        fs << "     <Placemark>\n"
              "       <name>"
           << xmtr[x].name
           << "</name>\n"
              "       <visibility>1</visibility>\n"
              "       <Style>\n"
              "       <IconStyle>\n"
              "        <Icon>\n"
              "          <href>root://icons/palette-5.png</href>\n"
              "          <x>224</x>\n"
              "          <y>224</y>\n"
              "          <w>32</w>\n"
              "          <h>32</h>\n"
              "        </Icon>\n"
              "       </IconStyle>\n"
              "       </Style>\n"
              "      <Point>\n"
              "        <extrude>1</extrude>\n"
              "        <altituem.demode>relativeToGround</altituem.demode>\n"
              "        <coordinates>"
           << (xmtr[x].lon < 180.0 ? -xmtr[x].lon : 360.0 - xmtr[x].lon) << ","
           << xmtr[x].lat << "," << xmtr[x].alt
           << "</coordinates>\n"
              "      </Point>\n"
              "     </Placemark>\n";
    }

    fs << "  </Folder>\n"
          "</kml>\n";

    fs.close();
}

void Image::WriteGeo(const std::string &geofile, const std::string &mapfile, double north,
                     double south, double east, double west, unsigned int width,
                     unsigned int height) {
    std::fstream fs;
    fs.open(geofile.c_str(), std::fstream::out);

    // TODO: Should we fail silently if we can't open. Shouldn't we WARN?
    if (!fs) {
        return;
    }

    // Always use 3 decimal places.
    fs.setf(std::ios::fixed, std::ios::floatfield);
    fs.precision(3);

    fs << "FILENAME\t" << mapfile
       << "\n"
          "#\t\tX\tY\tLong\t\tLat\n"
          "TIEPOINT\t0\t0\t"
       << west << "\t\t" << north
       << "\n"
          "TIEPOINT\t"
       << width - 1 << "\t" << height - 1 << "\t" << east << "\t\t" << south
       << "\n"
          "IMAGESIZE\t"
       << width << "\t" << height
       << "\n"
          "#\n# Auto Generated by "
       << SplatRun::splat_name << " v" << SplatRun::splat_version << "\n#\n";

    fs.close();
}
