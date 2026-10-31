# SPLAT! Utilities

Utilities for use with SPLAT! software are found under the
`utils` directory.  They include the following:

For required packages, CMake compile steps, and install targets (`splat`,
`mapterhorn2sdf` when libcurl/libwebp are present, `splat-batch`), see the
top-level [README.md](../README.md). Verified Mapterhorn / batch acceptance
results are in [docs/test_results.md](../docs/test_results.md).


## srtm2sdf
The `srtm2sdf` utility generates SPLAT Data Files (SDFs) from STS-99
Space Shuttle Topography Mission (SRTM) elevation data files.  This
data is of a much higher quality than that contained in older USGS
Digital Elevation Models of the same resolution.  However, many SRTM
Version 2 elevation models contain data "spikes", "voids", and "wells"
that are the consequence of the radar mapping process.

The `srtm2sdf` utility has the ability to detect and replace SRTM data
outliers with equivalent usgs2sdf derived SDF data (see `usgs2sdf` below).
If such data is not available, SRTM outliers are handled either through
adjacent pixel averaging, or by threshold limiting using user-specified
limits.  Of all three methods, the USGS-derived SDF replacement method
yields the best results.

The `srtm2sdf` utility processes SRTM-3 3-arc second resolution data
or use with SPLAT! operating in standard definition mode.

SRTM-3 Version 2 Elevation Data files may be downloaded from:

	http://dds.cr.usgs.gov/srtm/version2_1/SRTM3/

Files available at this site are ZIP compressed, and must be
uncompressed (using `unzip`, or `gunzip -S .zip`) prior to being
processed by `srtm2sdf`.

The `srtm2sdf` utility accepts command-line options as follows:

-d:  used to specify the directory path to the location of `usgs2sdf`
     derived SDF files that are to be used to replace outliers found
     in the SRTM data file.  The `-d` option overrides the default path
     specified in your `$HOME/.splat_path file`.

-n:  used to specify the elevation (in meters) below which SRTM data
     is either replaced with `usgs2sdf`-derived SDF data, or averaged
     among adjacent elevation data points.  The default threshold for
     the replacement limit is sea-level (0 meters).  Unless elevations
     below sea-level are known to exist for the region being
     processed by the `srtm2sdf` utility, the `-n` option need not be
     specified.

Some examples of srtm2sdf use:

    srtm2sdf N40W074.hgt

    srtm2sdf -d /cdrom/sdf N40W074.hgt

    srtm2sdf -d /dev/null N40W074.hgt (/dev/null prevents USGS data
		replacement from taking place)

    srtm2sdf -n -5 N40W074.hgt

In all cases, SDF files are written into the current working directory.

The srtm2sdf utility may also be used to convert 3-arc second SRTM data
in Band Interleaved by Line (.BIL) format for use with SPLAT!  This data 
is available via the web at: http://seamless.usgs.gov/website/seamless/

Once the region of the world has been selected at this site, select the
"Define Download Area By Coordinates" button under "Downloads".  Proceed
to request the download of the region(s) desired in 1 degree by 1 degree
regions only, and make sure bounding coordinates entered fall exactly on
whole numbers of latitude and longitude (no decimal fractions of a degree).

Select the "Add Area" button at the bottom.

On the next screen, select "Modify Data Request".  On the subsequent screen,
de-select the National Elevation Dataset (NED) format and select "SRTM 3 arc
sec - Shuttle Radar Topography Mission [Finished]".  Change the format from
ArcGrid to BIL, and from HTML to TXT.

Select the "Save Changes and Return To Summary" button.

Select the "Download" button, and save the file once it has been sent to
your web browser.

Uncompressing the file will generate a directory containing all files
contained within the downloaded archive.  Move into the directory and
invoke srtm2sdf as described above with the filename having the .bil
extension given as its argument.  Finally, move or copy the generated
.sdf file to your SPLAT! working directory.


## srtm2sdf-hd
The `srtm2sdf-hd` utility operates in an identical manner as `srtm2sdf`,
but is used to generate HD SDF files from SRTM-1 one-arc second
resolution data files for use with SPLAT! HD.  SRTM-1 data files
are available for the United States and its territories and
possessions, and may be downloaded from:

	http://dds.cr.usgs.gov/srtm/version2_1/SRTM1/


## usgs2sdf
The `usgs2sdf` utility takes as an argument the name of an uncompressed
and record delimited Digital Elevation Model Data (DEM) downloaded from
the US Geological Survey, and generates a SPLAT Data File (SDF) compatible
with SPLAT! Software.  `usgs2sdf` may be invoked manually, or via the
postdownload script.


## postdownload
`postdownload` is a front-end to the usgs2sdf utility.  `postdownload`
takes as an argument the name of the gzipped Digital Elevation Model
(DEM) downloaded from the US Geological Survey (ie: wilmington-w.gz).
`postdownload` uncompresses the DEM file, adds necessary record delimiters,
and invokes `usgs2sdf` to produce a SPLAT! Data File (SDF).

USGS Digital Elevation Models may be downloaded from:

    http://edcftp.cr.usgs.gov/pub/data/DEM/250/

Invoke `postdownload` with the name of each DEM file downloaded to
produce a database of SPLAT Data Files.


## citydecoder
This utility reads certain U.S. Census Bureau files to produce city/site
data files that can be imported into SPLAT! software to annotate
SPLAT!-generated maps.  Incorporated Places/Census Designated Places
data files for use with citydecoder may be downloaded from:

http://web.archive.org/web/20130201115142/http://www.census.gov/geo/www/cob/bdy_files.html

(Formerly: http://www.census.gov/geo/www/cob/bdy_files.html)

Similarly, County Subdivision files containing the names, locations,
and boundaries of cities, and smaller towns, townships, and boroughs
may be downloaded from:

http://web.archive.org/web/20130331172800/http://www.census.gov/geo/www/cob/cs2000.html

(Formerly: http://www.census.gov/geo/www/cob/cs2000.html)

and processed with the `citydecoder` utility. 

Please select the ARC/INFO Ungenerate (ASCII) Metadata Cartographic Boundary
Files from these sites and unzip them prior to processing them with
`citydecoder`:

	unzip -a pl34_d00_ascii.zip
	unzip -a cs34_d00_ascii.zip

U.S. Census files are cataloged by the two digit FIPS code for the region
(state) they represent.  A list of FIPS codes is included in fips.txt
under splat-1.4.1/utils for your convenience.

`citydecoder` takes as an argument the two-letter file prefix plus the FIPS
code of the region or state being processed.  For example:

	citydecoder pl34

reads files "pl34_d00.dat" and "pl34_d00a.dat" that were extracted after
the unzipping process, and generates a list of city names and geographical
coordinates for the state of New Jersey (FIPS code 34).  This data may be
sorted and written to a file (cities.nj.dat) in the following manner:

	citydecoder pl34 | sort > cities.nj.dat

In a similar manner, unzipped County Subdivision files may be processed
with the `citydecoder` utility to produce a file containing locations and
names of towns, townships, and boroughs:

	citydecoder cs34 | sort > townships.nj.dat

`citydecoder` can also process more than one file or file type per invocation,
and produce a merged output file as follows:

	citydecoder pl34 cs34 | sort > everything.in.nj.dat

Be advised that any redundancy that may exist between "cs" and "pl" files
will be reflected in the merged output file, so some manual editing of
the output file may be necessary.

 
 ## fontdata
The `fontdata` utility reads Slackware gzipped console font data
to create the `fontdata.h` file required for compilation of SPLAT!.
Font data of the type needed by this utility may be found under
`/usr/lib/kbd/consolefonts` (Slackware < 8), or under
`/usr/share/kbd/consolefonts` (Slackware >= 8.0).

A default `fontdata.h` file is already included in with SPLAT!, and is
a derivative of the s.fnt console font type available under Slackware.
fontdata takes as an argument the name of the file containing the
gzipped compressed console fonts:

	fontdata s.fnt.gz


## bearing
The bearing utility reads a pair of .qth files specified on the command
line, and returns the azimuth bearing and great circle path distance between
the two points specified.  A `-metric` switch is available so that distances
can be provided in kilometers rather than statute miles.  SPLAT! provides
similar distance and bearing information between two specific site locations.
The bearing utility, however, provides the information quickly and easily
over great distances without having to run SPLAT!

## aw3d30_2_srtmhgt.sh
aw3d30_2_srtmhgt.sh is a support utility used to convert AW3D30 
(ALOS Global Digital Surface Model 'ALOS World 3D - 30m) into 30m (1arc second)
srtm files which can then be processed by srtm2sdf-hd.

	aw3d30_2_srtmhgt.sh -s <srcdir> -d <destdir>

More information on the aw3d30 can be found at https://www.eorc.jaxa.jp/ALOS/en/aw3d30/index.htm 


## mapterhorn2sdf
`mapterhorn2sdf` generates SPLAT Data Files (SDFs) from Mapterhorn terrain
tiles stored in a PMTiles v3 archive, either a local file or a remote URL.
The Mapterhorn WebP tiles are lossless Terrarium-encoded elevation. The
output layout, file naming and value ordering mirror `srtm2sdf`, so the
generated files can be used interchangeably with SDFs made from SRTM data.
Existing output files are never overwritten.

It is built only when libcurl and libwebp are available. SPLAT! calls it
automatically when run with `-mapterhorn`, but it can also be used on its
own to prefetch terrain.

    mapterhorn2sdf [--hd] [--zoom N] [--source PATH|URL] [--outdir DIR] [--bz2] [--offline] [--no-despike] [--workers N] [--bbox minlat,minlon,maxlat,maxlon] <min_north> <lon_east>

Options:

    --hd               High definition (3600 ppd, files get an -hd suffix)
    --zoom N           Tile zoom level (default 12 with --hd, 10 otherwise)
    --source PATH|URL  PMTiles archive (default
                       https://download.mapterhorn.com/planet.pmtiles)
    --outdir DIR       Output directory (default .)
    --bz2              Write .sdf.bz2 files
    --offline          Refuse remote sources; for a local archive, fail and
                       list any required tiles that are missing
    --no-despike       Leave isolated elevation spikes untouched (default is
                       to remove multi-cell Copernicus-style cones that are
                       far above a surrounding ring; residual tails may remain)
    --workers N        Parallel workers for --bbox (default 1)
    --bbox ...         Generate every 1x1 degree page overlapping the box,
                       with longitudes in degrees EAST (replaces the
                       <min_north> <lon_east> arguments)

`<min_north>` is the integer latitude and `<lon_east>` the integer longitude
(degrees EAST, negative for west) of the south-west corner of one 1x1 degree
page.

### Degrees West

SPLAT! longitudes are degrees West (0-360). East longitudes, such as those in
Norway, are written as 360-lon in SDF file names, SDF headers and `.qth`
files. For example, the page covering 10-11E and 60-61N is
`60_61_349_350.sdf` (`60_61_349_350-hd.sdf` in HD mode), with the header

    350     (max_west)
    60      (min_north)
    349     (min_west)
    61      (max_north)

and is generated with `mapterhorn2sdf 60 10`. A `.qth` file for a site at
10.75E would contain the longitude 349.25.

### Prefetching a region

For production use, prefer a local PMTiles archive so that runs never depend
on the remote server. A subset can be cut out of the planet archive with the
`pmtiles` tool, or a regional extract can be downloaded.

**Bbox order:** `pmtiles extract --bbox` uses **longitude, latitude**:
`MIN_LON,MIN_LAT,MAX_LON,MAX_LAT`. `mapterhorn2sdf --bbox` uses **latitude,
longitude**: `minlat,minlon,maxlat,maxlon` (longitudes in degrees EAST).
The two tools use opposite axis order. Putting coordinates in the wrong order
often yields an empty or misplaced extract with no error, so check the order
next to each command below.

Norway mainland plus roughly 150 km margin for SPLAT! coverage (does not
include Svalbard; use a separate small extract for Svalbard repeaters):

    # lon,lat: south into Sweden (~56.5N), west margin, east into Russia
    # for Finnmark (~36E), north to ~72.5N (Svalbard is ~78N)
    pmtiles extract planet.pmtiles norway.pmtiles --bbox=2,56.5,36,72.5

    # lat,lon (degrees EAST for lon): same box as above
    mapterhorn2sdf --hd --source norway.pmtiles --outdir /data/sdf --bz2 \
        --workers 4 --bbox 56.5,2,72.5,36

A full HD prefetch of that box is 400+ 1x1-degree pages. Uncompressed HD SDF
for the region is tens of gigabytes; prefer `--bz2` and plan disk space. Sea
tiles compress well.

Runs with `splat -mapterhorn -mt-offline` (or `mapterhorn2sdf --offline`)
then never use the network and fail with a list of any missing pages. This
is suitable for air-gapped machines after the prefetch.

Attribution is REQUIRED when using Mapterhorn data:
https://mapterhorn.com/attribution


## splat-batch
`splat-batch` (`utils/splat-batch`, implemented in `utils/splat_batch`) is a
batch coverage driver. It:

* reads repeater lists from `.joz` (JOSM session), GeoJSON or CSV files;
* filters out `qrt` entries, data-only modulations and entries with
  disagreeing positions;
* writes a `.qth`, `.lrp` and `.lcf` file for each job;
* prefetches the needed terrain (with `--mapterhorn`);
* runs SPLAT! on all jobs in parallel;
* polygonizes the resulting "hear" and "talk" areas;
* writes one GeoJSON and one FlatGeobuf file per county;
* caches results by a hash of the job parameters, so unchanged jobs are
  skipped on later runs.

Example:

    utils/splat-batch --input repeaters.joz --out-dir ./out \
      --mapterhorn --mt-source /data/norway.pmtiles --mt-cache /data/sdf \
      --splat-bin ./build/splat --hd --workers 4

Dependencies: Python 3, numpy, shapely, and the GDAL command-line tools
(`gdal_translate`, `gdal_polygonize`, `ogr2ogr`).

See `utils/splat_batch/README.md` for the full description of inputs,
configuration, caching and options.
