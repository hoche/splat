/**************************************************************
 **                                                          **
 **  mapterhorn2sdf: Generates SPLAT! elevation data files   **
 **  (.sdf / .sdf.bz2) from Mapterhorn Terrarium WebP tiles  **
 **  stored in a local or remote PMTiles v3 archive.         **
 **                                                          **
 **  The output layout, file naming and value ordering       **
 **  mirror utils/srtm2sdf.c so that the generated files     **
 **  are interchangeable with those produced from SRTM       **
 **  .hgt data.                                              **
 **                                                          **
 **  Compile like this:                                      **
 **                                                          **
 **  c++ -std=c++17 -O2 -Ithird_party/pmtiles                **
 **      -I/usr/include/webp utils/mapterhorn2sdf.cpp        **
 **      -o mapterhorn2sdf -lcurl -lwebp -lz -lbz2 -pthread  **
 **                                                          **
 **  Terrain data: Mapterhorn                                **
 **  (https://mapterhorn.com/attribution)                    **
 **                                                          **
 **************************************************************/

#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <bzlib.h>
#include <curl/curl.h>
#include <webp/decode.h>
#include <zlib.h>

#include "../third_party/pmtiles/pmtiles.hpp"

namespace fs = std::filesystem;

namespace {

constexpr const char *kDefaultSource =
    "https://download.mapterhorn.com/planet.pmtiles";
constexpr const char *kAttribution =
    "Terrain data: Mapterhorn (https://mapterhorn.com/attribution)";

constexpr int kTileSize = 512;         /* Mapterhorn tiles are 512x512    */
constexpr int kMaxDirDepth = 3;        /* PMTiles leaf directory depth cap */
constexpr size_t kMaxInflate = 1ULL << 30;
constexpr size_t kMaxTileBytes = 64ULL << 20;
constexpr double kMercatorMaxLat = 85.0511287798;

/* ------------------------------------------------------------------ */
/* Logging                                                            */
/* ------------------------------------------------------------------ */

std::mutex g_log_mutex;

void LogOut(const std::string &msg) {
    std::lock_guard<std::mutex> lock(g_log_mutex);
    std::fputs(msg.c_str(), stdout);
    std::fputc('\n', stdout);
    std::fflush(stdout);
}

void LogErr(const std::string &msg) {
    std::lock_guard<std::mutex> lock(g_log_mutex);
    std::fputs(msg.c_str(), stderr);
    std::fputc('\n', stderr);
    std::fflush(stderr);
}

/* ------------------------------------------------------------------ */
/* Compression helpers                                                */
/* ------------------------------------------------------------------ */

/* Inflate a gzip (or zlib) stream held in memory. */
std::string Gunzip(const std::string &in) {
    if (in.size() > UINT_MAX)
        throw std::runtime_error("compressed block too large");

    z_stream zs;
    std::memset(&zs, 0, sizeof(zs));

    /* windowBits 15 + 32: automatic gzip / zlib header detection */
    if (inflateInit2(&zs, 15 + 32) != Z_OK)
        throw std::runtime_error("inflateInit2 failed");

    zs.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(in.data()));
    zs.avail_in = static_cast<uInt>(in.size());

    std::string out;
    char buf[65536];
    int ret;

    do {
        zs.next_out = reinterpret_cast<Bytef *>(buf);
        zs.avail_out = sizeof(buf);
        ret = inflate(&zs, Z_NO_FLUSH);

        if (ret != Z_OK && ret != Z_STREAM_END) {
            inflateEnd(&zs);
            throw std::runtime_error(
                std::string("gzip decompression failed: ") +
                (zs.msg ? zs.msg : "corrupt or truncated data"));
        }

        out.append(buf, sizeof(buf) - zs.avail_out);

        if (out.size() > kMaxInflate) {
            inflateEnd(&zs);
            throw std::runtime_error("decompressed block exceeds size limit");
        }
    } while (ret != Z_STREAM_END);

    inflateEnd(&zs);
    return out;
}

/* Undo the PMTiles compression layer identified by 'compression'. */
std::string Decompress(std::string data, uint8_t compression) {
    switch (compression) {
    case pmtiles::COMPRESSION_NONE:
        return data;
    case pmtiles::COMPRESSION_GZIP:
        return Gunzip(data);
    case pmtiles::COMPRESSION_UNKNOWN:
        /* Sniff for a gzip magic number, otherwise pass through */
        if (data.size() >= 2 && static_cast<uint8_t>(data[0]) == 0x1f &&
            static_cast<uint8_t>(data[1]) == 0x8b)
            return Gunzip(data);
        return data;
    default:
        throw std::runtime_error(
            "unsupported PMTiles compression (only none and gzip are "
            "supported)");
    }
}

/* ------------------------------------------------------------------ */
/* HTTP access (libcurl)                                              */
/* ------------------------------------------------------------------ */

struct CurlHandle {
    CURL *h;
    CurlHandle() : h(curl_easy_init()) {}
    ~CurlHandle() {
        if (h)
            curl_easy_cleanup(h);
    }
};

CURL *ThreadCurl() {
    thread_local CurlHandle handle;
    if (!handle.h)
        throw std::runtime_error("curl_easy_init failed");
    return handle.h;
}

struct HttpBody {
    std::string data;
    size_t limit = 0;
    bool overflow = false;
};

size_t HttpWrite(char *ptr, size_t size, size_t nmemb, void *userdata) {
    HttpBody *body = static_cast<HttpBody *>(userdata);
    size_t n = size * nmemb;

    if (body->data.size() + n > body->limit) {
        body->overflow = true;
        return 0; /* abort transfer */
    }

    body->data.append(ptr, n);
    return n;
}

struct HttpResult {
    long status = 0;
    std::string body;
};

/* Performs a GET (optionally with a byte range) with bounded retries.
 * 'max_body' limits the accepted body size. */
HttpResult HttpFetch(const std::string &url, const std::string *range,
                     size_t max_body) {
    const int attempts = 4;
    std::string last_error;

    for (int attempt = 0; attempt < attempts; attempt++) {
        if (attempt > 0)
            std::this_thread::sleep_for(
                std::chrono::milliseconds(500 << (attempt - 1)));

        CURL *h = ThreadCurl();
        curl_easy_reset(h);

        HttpBody body;
        body.limit = max_body;
        char errbuf[CURL_ERROR_SIZE];
        errbuf[0] = 0;

        curl_easy_setopt(h, CURLOPT_URL, url.c_str());
        curl_easy_setopt(h, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(h, CURLOPT_MAXREDIRS, 5L);
        curl_easy_setopt(h, CURLOPT_CONNECTTIMEOUT, 30L);
        curl_easy_setopt(h, CURLOPT_TIMEOUT, 180L);
        curl_easy_setopt(h, CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(h, CURLOPT_USERAGENT, "mapterhorn2sdf (SPLAT!)");
        curl_easy_setopt(h, CURLOPT_ERRORBUFFER, errbuf);
        curl_easy_setopt(h, CURLOPT_WRITEFUNCTION, HttpWrite);
        curl_easy_setopt(h, CURLOPT_WRITEDATA, &body);

        if (range)
            curl_easy_setopt(h, CURLOPT_RANGE, range->c_str());

        CURLcode rc = curl_easy_perform(h);

        if (rc == CURLE_WRITE_ERROR && body.overflow)
            throw std::runtime_error(
                "unexpected response size from " + url +
                " (server may not honor HTTP Range requests)");

        if (rc != CURLE_OK) {
            last_error = errbuf[0] ? errbuf : curl_easy_strerror(rc);
            continue; /* transient network error: retry */
        }

        long status = 0;
        curl_easy_getinfo(h, CURLINFO_RESPONSE_CODE, &status);

        if (status == 429 || status >= 500) {
            last_error = "HTTP " + std::to_string(status);
            continue;
        }

        HttpResult result;
        result.status = status;
        result.body = std::move(body.data);
        return result;
    }

    throw std::runtime_error("request failed for " + url + ": " + last_error);
}

/* ------------------------------------------------------------------ */
/* Byte sources (local file or remote URL)                            */
/* ------------------------------------------------------------------ */

class ByteSource {
  public:
    virtual ~ByteSource() = default;
    virtual std::string Read(uint64_t offset, uint64_t length) = 0;
};

class LocalSource : public ByteSource {
  public:
    explicit LocalSource(const std::string &path) : path_(path) {
        fp_ = std::fopen(path.c_str(), "rb");
        if (!fp_)
            throw std::runtime_error("cannot open PMTiles file \"" + path +
                                     "\": " + std::strerror(errno));
    }

    ~LocalSource() override {
        if (fp_)
            std::fclose(fp_);
    }

    std::string Read(uint64_t offset, uint64_t length) override {
        std::string out(static_cast<size_t>(length), '\0');
        std::lock_guard<std::mutex> lock(mutex_);

#ifdef _WIN32
        int rc = _fseeki64(fp_, static_cast<__int64>(offset), SEEK_SET);
#else
        int rc = fseeko(fp_, static_cast<off_t>(offset), SEEK_SET);
#endif
        if (rc != 0)
            throw std::runtime_error("seek failed in \"" + path_ + "\"");

        if (length > 0 && std::fread(&out[0], 1, out.size(), fp_) != out.size())
            throw std::runtime_error("short read from \"" + path_ +
                                     "\" (truncated archive?)");
        return out;
    }

  private:
    std::string path_;
    FILE *fp_ = nullptr;
    std::mutex mutex_;
};

class HttpSource : public ByteSource {
  public:
    explicit HttpSource(const std::string &url) : url_(url) {}

    std::string Read(uint64_t offset, uint64_t length) override {
        if (length == 0)
            return std::string();

        std::string range =
            std::to_string(offset) + "-" + std::to_string(offset + length - 1);
        HttpResult res = HttpFetch(url_, &range, static_cast<size_t>(length));

        if (res.status == 206 && res.body.size() == length)
            return std::move(res.body);

        if (res.status == 200)
            throw std::runtime_error(
                "server ignored the HTTP Range request for " + url_);
        if (res.status == 416)
            throw std::runtime_error("requested range is outside " + url_);
        if (res.status == 206)
            throw std::runtime_error("short range response from " + url_);

        throw std::runtime_error("HTTP " + std::to_string(res.status) +
                                 " while reading " + url_);
    }

  private:
    std::string url_;
};

/* ------------------------------------------------------------------ */
/* Tile providers                                                     */
/* ------------------------------------------------------------------ */

class TileProvider {
  public:
    virtual ~TileProvider() = default;

    /* Returns true and fills 'webp' if the tile exists, false if the
     * source has no such tile.  Throws on I/O or format errors. */
    virtual bool Fetch(int z, int x, int y, std::string &webp) = 0;

    /* Cheap existence test (no tile payload transfer where possible). */
    virtual bool Probe(int z, int x, int y) = 0;

    /* Inclusive zoom range available, or false if unknown. */
    virtual bool ZoomRange(int &min_z, int &max_z) const = 0;
};

class PMTilesProvider : public TileProvider {
  public:
    explicit PMTilesProvider(std::unique_ptr<ByteSource> src)
        : src_(std::move(src)) {
        std::string head = src_->Read(0, 127);

        try {
            header_ = pmtiles::deserialize_header(head);
        } catch (const pmtiles::pmtiles_magic_number_exception &) {
            throw std::runtime_error("source is not a PMTiles archive "
                                     "(bad magic number)");
        } catch (const pmtiles::pmtiles_version_exception &) {
            throw std::runtime_error("unsupported PMTiles version (v3 "
                                     "required)");
        }

        if (header_.tile_type != pmtiles::TILETYPE_WEBP)
            throw std::runtime_error(
                "PMTiles archive does not contain WebP tiles");

        if (header_.root_dir_bytes == 0 ||
            header_.root_dir_bytes > (64ULL << 20))
            throw std::runtime_error("invalid root directory size in header");
    }

    bool Fetch(int z, int x, int y, std::string &webp) override {
        uint64_t offset;
        uint32_t length;

        if (!Locate(z, x, y, offset, length))
            return false;

        webp = Decompress(src_->Read(offset, length),
                          header_.tile_compression);
        return true;
    }

    bool Probe(int z, int x, int y) override {
        uint64_t offset;
        uint32_t length;
        return Locate(z, x, y, offset, length);
    }

    bool ZoomRange(int &min_z, int &max_z) const override {
        min_z = header_.min_zoom;
        max_z = header_.max_zoom;
        return true;
    }

  private:
    typedef std::shared_ptr<const std::vector<pmtiles::entryv3>> DirPtr;

    DirPtr LoadDirectory(uint64_t offset, uint64_t length) {
        {
            std::lock_guard<std::mutex> lock(dir_mutex_);
            auto it = dir_cache_.find(offset);
            if (it != dir_cache_.end())
                return it->second;
        }

        if (length == 0 || length > (256ULL << 20))
            throw std::runtime_error("invalid PMTiles directory length");

        std::string raw = src_->Read(offset, length);
        std::string dir = Decompress(std::move(raw), header_.internal_compression);

        DirPtr parsed;
        try {
            parsed = std::make_shared<const std::vector<pmtiles::entryv3>>(
                pmtiles::deserialize_directory(dir));
        } catch (const std::exception &e) {
            throw std::runtime_error(std::string("corrupt PMTiles directory: ") +
                                     e.what());
        }

        std::lock_guard<std::mutex> lock(dir_mutex_);
        dir_cache_[offset] = parsed;
        return parsed;
    }

    bool Locate(int z, int x, int y, uint64_t &offset, uint32_t &length) {
        uint64_t tile_id;

        try {
            tile_id = pmtiles::zxy_to_tileid(static_cast<uint8_t>(z),
                                             static_cast<uint32_t>(x),
                                             static_cast<uint32_t>(y));
        } catch (const std::exception &e) {
            throw std::runtime_error("invalid tile address " +
                                     std::to_string(z) + "/" +
                                     std::to_string(x) + "/" +
                                     std::to_string(y) + ": " + e.what());
        }

        uint64_t dir_offset = header_.root_dir_offset;
        uint64_t dir_length = header_.root_dir_bytes;

        for (int depth = 0; depth <= kMaxDirDepth; depth++) {
            DirPtr dir = LoadDirectory(dir_offset, dir_length);
            pmtiles::entryv3 entry = pmtiles::find_tile(*dir, tile_id);

            if (entry.length == 0)
                return false;

            if (entry.run_length > 0) {
                offset = header_.tile_data_offset + entry.offset;
                length = entry.length;
                return true;
            }

            /* Entry points to a leaf directory */
            dir_offset = header_.leaf_dirs_offset + entry.offset;
            dir_length = entry.length;
        }

        throw std::runtime_error(
            "PMTiles leaf directory depth exceeds the supported maximum");
    }

    std::unique_ptr<ByteSource> src_;
    pmtiles::headerv3 header_;
    std::mutex dir_mutex_;
    std::unordered_map<uint64_t, DirPtr> dir_cache_;
};

/* Direct tile server access, e.g. https://tiles.mapterhorn.com/{z}/{x}/{y}.webp */
class DirectUrlProvider : public TileProvider {
  public:
    explicit DirectUrlProvider(const std::string &tmpl) : tmpl_(tmpl) {
        if (tmpl_.find("{z}") == std::string::npos ||
            tmpl_.find("{x}") == std::string::npos ||
            tmpl_.find("{y}") == std::string::npos)
            throw std::runtime_error(
                "--tiles-url must contain {z}, {x} and {y} placeholders");
    }

    bool Fetch(int z, int x, int y, std::string &webp) override {
        HttpResult res = HttpFetch(Expand(z, x, y), nullptr, kMaxTileBytes);

        if (res.status == 404 || res.status == 403 || res.status == 410)
            return false;
        if (res.status != 200)
            throw std::runtime_error("HTTP " + std::to_string(res.status) +
                                     " fetching tile " + std::to_string(z) +
                                     "/" + std::to_string(x) + "/" +
                                     std::to_string(y));
        webp = std::move(res.body);
        return true;
    }

    bool Probe(int, int, int) override { return true; }

    bool ZoomRange(int &, int &) const override { return false; }

  private:
    std::string Expand(int z, int x, int y) const {
        std::string url = tmpl_;
        Replace(url, "{z}", std::to_string(z));
        Replace(url, "{x}", std::to_string(x));
        Replace(url, "{y}", std::to_string(y));
        return url;
    }

    static void Replace(std::string &s, const std::string &key,
                        const std::string &val) {
        size_t pos = 0;
        while ((pos = s.find(key, pos)) != std::string::npos) {
            s.replace(pos, key.size(), val);
            pos += val.size();
        }
    }

    std::string tmpl_;
};

/* ------------------------------------------------------------------ */
/* Terrarium decoding and tile cache                                  */
/* ------------------------------------------------------------------ */

std::atomic_flag g_lossy_warned = ATOMIC_FLAG_INIT;

/* Decodes a Terrarium WebP tile to metres, clamping below-sea-level
 * samples to zero to match the sea level convention of SRTM SDF data. */
void DecodeTile(const std::string &webp, int z, int x, int y,
                std::vector<float> &elev) {
    const uint8_t *data = reinterpret_cast<const uint8_t *>(webp.data());
    std::string label =
        std::to_string(z) + "/" + std::to_string(x) + "/" + std::to_string(y);

    WebPBitstreamFeatures feat;
    if (WebPGetFeatures(data, webp.size(), &feat) != VP8_STATUS_OK)
        throw std::runtime_error("tile " + label + " is not a valid WebP");

    if (feat.has_animation)
        throw std::runtime_error("tile " + label + " is an animated WebP");

    if (feat.width != kTileSize || feat.height != kTileSize)
        throw std::runtime_error(
            "tile " + label + " has unexpected size " +
            std::to_string(feat.width) + "x" + std::to_string(feat.height) +
            " (expected 512x512)");

    /* feat.format: 0 = undefined/mixed, 1 = lossy (VP8), 2 = lossless (VP8L) */
    if (feat.format != 2 && !g_lossy_warned.test_and_set())
        LogErr("Warning: tile " + label +
               " is not lossless WebP; Terrarium elevations decoded from lossy "
               "tiles contain compression artifacts.");

    int w = 0, h = 0;
    uint8_t *rgba = WebPDecodeRGBA(data, webp.size(), &w, &h);
    if (!rgba)
        throw std::runtime_error("failed to decode WebP tile " + label);

    elev.resize(static_cast<size_t>(w) * h);

    for (size_t i = 0; i < elev.size(); i++) {
        const uint8_t *p = rgba + i * 4;
        double v = (p[0] * 256.0 + p[1] + p[2] / 256.0) - 32768.0;
        elev[i] = v < 0.0 ? 0.0f : static_cast<float>(v);
    }

    WebPFree(rgba);
}

struct CacheStats {
    size_t tiles_fetched = 0;
    size_t tiles_missing = 0;
    size_t bytes_fetched = 0;
    std::vector<std::string> missing_examples;
};

/* Per-worker LRU cache of decoded tiles at a fixed zoom level. */
class TileCache {
  public:
    TileCache(TileProvider &provider, int zoom, size_t capacity)
        : provider_(provider), zoom_(zoom),
          capacity_(std::max<size_t>(capacity, 8)) {}

    /* Elevation at a global pixel position. 'px' must already be wrapped
     * into [0, N) and 'py' clamped into [0, N), N = tiles * 512. */
    float Pixel(int64_t px, int64_t py) {
        int tx = static_cast<int>(px / kTileSize);
        int ty = static_cast<int>(py / kTileSize);
        Entry *e = Get(tx, ty);

        if (e->missing)
            return 0.0f;

        return e->elev[static_cast<size_t>(py % kTileSize) * kTileSize +
                       static_cast<size_t>(px % kTileSize)];
    }

    const CacheStats &Stats() const { return stats_; }

  private:
    struct Entry {
        std::vector<float> elev;
        bool missing = false;
        uint64_t used = 0;
    };

    static uint64_t Key(int tx, int ty) {
        return (static_cast<uint64_t>(ty) << 32) | static_cast<uint32_t>(tx);
    }

    Entry *Get(int tx, int ty) {
        uint64_t key = Key(tx, ty);

        if (last_ && key == last_key_) {
            last_->used = ++clock_;
            return last_;
        }

        auto it = map_.find(key);
        if (it == map_.end()) {
            if (map_.size() >= capacity_)
                EvictOldest();
            it = map_.emplace(key, Load(tx, ty)).first;
        }

        it->second.used = ++clock_;
        last_ = &it->second;
        last_key_ = key;
        return last_;
    }

    Entry Load(int tx, int ty) {
        Entry e;
        std::string webp;

        if (!provider_.Fetch(zoom_, tx, ty, webp)) {
            e.missing = true;
            stats_.tiles_missing++;
            if (stats_.missing_examples.size() < 5)
                stats_.missing_examples.push_back(
                    std::to_string(zoom_) + "/" + std::to_string(tx) + "/" +
                    std::to_string(ty));
            return e;
        }

        stats_.tiles_fetched++;
        stats_.bytes_fetched += webp.size();
        DecodeTile(webp, zoom_, tx, ty, e.elev);
        return e;
    }

    void EvictOldest() {
        auto oldest = map_.begin();
        for (auto it = map_.begin(); it != map_.end(); ++it)
            if (it->second.used < oldest->second.used)
                oldest = it;

        last_ = nullptr;
        map_.erase(oldest);
    }

    TileProvider &provider_;
    int zoom_;
    size_t capacity_;
    std::unordered_map<uint64_t, Entry> map_;
    Entry *last_ = nullptr;
    uint64_t last_key_ = 0;
    uint64_t clock_ = 0;
    CacheStats stats_;
};

/* ------------------------------------------------------------------ */
/* Geometry                                                           */
/* ------------------------------------------------------------------ */

/* Normalised Web Mercator Y in [0, 1] (0 at the north edge). */
double MercatorY(double lat_deg) {
    double lat = lat_deg * M_PI / 180.0;
    return (1.0 - std::log(std::tan(lat) + 1.0 / std::cos(lat)) / M_PI) / 2.0;
}

double MercatorX(double lon_east) { return (lon_east + 180.0) / 360.0; }

int64_t FloorDiv(int64_t a, int64_t b) {
    int64_t q = a / b;
    if ((a % b != 0) && ((a < 0) != (b < 0)))
        q--;
    return q;
}

int64_t WrapMod(int64_t a, int64_t n) {
    int64_t r = a % n;
    return r < 0 ? r + n : r;
}

/* Converts the east-longitude of a page's south-west corner into the
 * SPLAT! west-degree pair, mirroring the HGT handling in srtm2sdf.c. */
void WestRange(int lon_east, int &max_west, int &min_west) {
    int raw = (lon_east >= 0) ? 360 - lon_east : -lon_east;

    min_west = raw - 1;
    max_west = (raw == 360) ? 0 : raw;
}

/* ------------------------------------------------------------------ */
/* Configuration and output                                           */
/* ------------------------------------------------------------------ */

struct Config {
    bool hd = false;
    int zoom = -1;
    std::string source = kDefaultSource;
    std::string tiles_url;
    std::string outdir = ".";
    bool bz2 = false;
    bool offline = false;
    bool despike = true;
    int workers = 1;
    std::string delimiter = "_";
};

struct Page {
    int lat; /* integer south-west corner latitude (min_north) */
    int lon; /* integer south-west corner longitude, degrees east */
};

std::string PageName(const Page &page, const Config &cfg) {
    int max_west, min_west;
    WestRange(page.lon, max_west, min_west);

    const std::string &d = cfg.delimiter;
    return std::to_string(page.lat) + d + std::to_string(page.lat + 1) + d +
           std::to_string(min_west) + d + std::to_string(max_west) +
           (cfg.hd ? "-hd" : "") + ".sdf" + (cfg.bz2 ? ".bz2" : "");
}

/* Plain or bzip2 output stream; writes to a temporary path that is
 * renamed on Close() so that an interrupted run never leaves a partial
 * file behind that would later be mistaken for a cached result. */
class OutputFile {
  public:
    OutputFile(const fs::path &path, bool bz2) : bz2_(bz2) {
        fp_ = std::fopen(path.string().c_str(), "wb");
        if (!fp_)
            throw std::runtime_error("cannot create \"" + path.string() +
                                     "\": " + std::strerror(errno));

        if (bz2_) {
            int bzerr = BZ_OK;
            bzf_ = BZ2_bzWriteOpen(&bzerr, fp_, 9, 0, 0);
            if (bzerr != BZ_OK) {
                std::fclose(fp_);
                fp_ = nullptr;
                throw std::runtime_error("BZ2_bzWriteOpen failed");
            }
        }
    }

    ~OutputFile() {
        if (bzf_) {
            int bzerr = BZ_OK;
            BZ2_bzWriteClose(&bzerr, bzf_, 1, nullptr, nullptr);
        }
        if (fp_)
            std::fclose(fp_);
    }

    OutputFile(const OutputFile &) = delete;
    OutputFile &operator=(const OutputFile &) = delete;

    void Write(const char *data, size_t len) {
        if (bz2_) {
            int bzerr = BZ_OK;
            BZ2_bzWrite(&bzerr, bzf_, const_cast<char *>(data),
                        static_cast<int>(len));
            if (bzerr != BZ_OK)
                throw std::runtime_error("bzip2 write error");
        } else if (std::fwrite(data, 1, len, fp_) != len) {
            throw std::runtime_error("write error (disk full?)");
        }
    }

    void Close() {
        if (bz2_) {
            int bzerr = BZ_OK;
            BZ2_bzWriteClose(&bzerr, bzf_, 0, nullptr, nullptr);
            bzf_ = nullptr;
            if (bzerr != BZ_OK)
                throw std::runtime_error("bzip2 finalisation error");
        }

        int rc = std::fclose(fp_);
        fp_ = nullptr;
        if (rc != 0)
            throw std::runtime_error("write error on close (disk full?)");
    }

  private:
    bool bz2_;
    FILE *fp_ = nullptr;
    BZFILE *bzf_ = nullptr;
};

/* ------------------------------------------------------------------ */
/* SDF generation                                                     */
/* ------------------------------------------------------------------ */

struct TileRange {
    int64_t x_lo, x_hi; /* unwrapped tile column range */
    int64_t y_lo, y_hi; /* clamped tile row range      */
};

/* Tiles touched by the bilinear samples of one page. */
TileRange PageTileRange(const Page &page, const Config &cfg, int zoom) {
    const int ippd = cfg.hd ? 3600 : 1200;
    const double ppd = ippd;
    const double N = static_cast<double>(kTileSize) * (1LL << zoom);
    const int64_t Ni = static_cast<int64_t>(N);

    double lon_lo = page.lon;
    double lon_hi = page.lon + (ippd - 1) / ppd;
    double lat_lo = page.lat;
    double lat_hi = page.lat + (ippd - 1) / ppd;

    int64_t x0 = static_cast<int64_t>(std::floor(MercatorX(lon_lo) * N - 0.5));
    int64_t x1 =
        static_cast<int64_t>(std::floor(MercatorX(lon_hi) * N - 0.5)) + 1;
    int64_t y0 = static_cast<int64_t>(std::floor(MercatorY(lat_hi) * N - 0.5));
    int64_t y1 =
        static_cast<int64_t>(std::floor(MercatorY(lat_lo) * N - 0.5)) + 1;

    y0 = std::clamp<int64_t>(y0, 0, Ni - 1);
    y1 = std::clamp<int64_t>(y1, 0, Ni - 1);

    return {FloorDiv(x0, kTileSize), FloorDiv(x1, kTileSize),
            FloorDiv(y0, kTileSize), FloorDiv(y1, kTileSize)};
}

/* Offline mode: verify every required tile exists before any work. */
void CheckOfflineTiles(TileProvider &provider, const Page &page,
                       const Config &cfg, int zoom, const std::string &name) {
    TileRange r = PageTileRange(page, cfg, zoom);
    const int64_t n = 1LL << zoom;
    std::vector<std::string> missing;

    for (int64_t ty = r.y_lo; ty <= r.y_hi; ty++)
        for (int64_t tx = r.x_lo; tx <= r.x_hi; tx++) {
            int64_t wx = WrapMod(tx, n);
            if (!provider.Probe(zoom, static_cast<int>(wx),
                                static_cast<int>(ty)))
                missing.push_back(std::to_string(zoom) + "/" +
                                  std::to_string(wx) + "/" +
                                  std::to_string(ty));
        }

    if (missing.empty())
        return;

    std::string msg = "offline mode: " + std::to_string(missing.size()) +
                      " tile(s) required for " + name +
                      " are missing from the archive:";
    const size_t shown = std::min<size_t>(missing.size(), 50);
    for (size_t i = 0; i < shown; i++)
        msg += "\n  " + missing[i];
    if (shown < missing.size())
        msg += "\n  ... and " + std::to_string(missing.size() - shown) +
               " more";

    throw std::runtime_error(msg);
}

/* Elevation despike
 *
 * Global DEM mosaics derived from Copernicus-style radar data (which
 * Mapterhorn includes in places) occasionally contain spikes: a single
 * cell or a multi-cell cone or blob standing far above its surroundings.
 * One example is a cone of about 494 m over the Randsfjorden lake in
 * Norway, where the surrounding terrain and water surface are about
 * 135 m; the artefact is a blob several cells across with a tail that
 * falls off over a few tens of cells. Such artefacts produce bogus
 * line-of-sight obstructions in propagation predictions.
 *
 * Because the artefact is several cells wide, comparing a cell with its
 * eight neighbours is not enough: the neighbours are part of the blob.
 * Instead each cell is compared with the elevations on a square ring at
 * Chebyshev distance kDespikeRadius, i.e. the cells where
 * max(|di|,|dj|) equals the radius. The ring lies outside the spike
 * core and is cheaper to sample than the filled disk. Where the ring is
 * clipped by the page edge and has too few samples, every cell of the
 * annulus from half the radius to the radius is used instead.
 *
 * The filter is deliberately conservative. A cell is replaced by the
 * ring median only if it is both more than kSpikeAboveMax metres above
 * the highest ring cell and more than kSpikeAboveMedian metres above the
 * ring median. Real peaks, ridges and slopes are connected to higher
 * terrain within the ring and are left untouched, as are flat lake and
 * sea surfaces. Each pass reads from an unmodified copy so replacements
 * never influence the decisions made for neighbouring cells within the
 * same pass. Up to kDespikeMaxPasses passes are made, stopping as soon as
 * one changes nothing, so that a blob wider than the ring is peeled from
 * the top down. */

constexpr int kDespikeRadius = 6;
constexpr int kDespikeMaxPasses = 3;
constexpr int kSpikeAboveMax = 100;
constexpr int kSpikeAboveMedian = 150;

struct DespikeStats {
    size_t removed = 0; /* replacements over all passes */
    int max_excess = 0; /* largest (replaced - replacement) in metres */
    int passes = 0;     /* passes that changed at least one cell */
};

/* One despike pass. Reads src, writes replacements into dst and returns the
 * number of cells replaced. */
size_t DespikePass(const std::vector<int> &src, std::vector<int> &dst, int n,
                   DespikeStats &stats) {
    constexpr int R = kDespikeRadius;
    /* A full ring has 8*R cells; below half of that use the annulus */
    constexpr size_t kMinRing = 4 * R;
    /* With fewer samples than this the cell is left alone */
    constexpr size_t kMinSamples = 8;

    std::vector<int> samples;
    samples.reserve(static_cast<size_t>(4 * R + 1) * (2 * R + 1));
    size_t changed = 0;

    for (int i = 0; i < n; i++) {
        const int i_lo = std::max(i - R, 0);
        const int i_hi = std::min(i + R, n - 1);

        for (int j = 0; j < n; j++) {
            const size_t idx = static_cast<size_t>(i) * n + j;
            const int elev = src[idx];
            const int j_lo = std::max(j - R, 0);
            const int j_hi = std::min(j + R, n - 1);

            /* Pass 1: maximum (cheap) over the ring border */
            int ring_max = INT_MIN;
            size_t cnt = 0;

            for (int ii = i_lo; ii <= i_hi; ii++) {
                const int *row = &src[static_cast<size_t>(ii) * n];

                if (ii == i - R || ii == i + R) {
                    for (int jj = j_lo; jj <= j_hi; jj++) {
                        ring_max = std::max(ring_max, row[jj]);
                        cnt++;
                    }
                } else {
                    if (j - R >= 0) {
                        ring_max = std::max(ring_max, row[j - R]);
                        cnt++;
                    }
                    if (j + R < n) {
                        ring_max = std::max(ring_max, row[j + R]);
                        cnt++;
                    }
                }
            }

            /* Clipped ring too sparse: use the annulus R/2..R instead */
            const bool annulus = cnt < kMinRing;
            const int d_min = annulus ? R / 2 : R;

            if (annulus) {
                ring_max = INT_MIN;
                cnt = 0;
                for (int ii = i_lo; ii <= i_hi; ii++) {
                    const int *row = &src[static_cast<size_t>(ii) * n];
                    const int di = std::abs(ii - i);
                    for (int jj = j_lo; jj <= j_hi; jj++) {
                        if (std::max(di, std::abs(jj - j)) < d_min)
                            continue;
                        ring_max = std::max(ring_max, row[jj]);
                        cnt++;
                    }
                }
            }

            if (cnt < kMinSamples || elev <= ring_max + kSpikeAboveMax)
                continue;

            /* Pass 2 (candidates only): gather the samples for the median.
             * For the ring d_min == R selects the border; for the annulus
             * it selects every cell at distance R/2 or more. */
            samples.clear();
            for (int ii = i_lo; ii <= i_hi; ii++) {
                const int *row = &src[static_cast<size_t>(ii) * n];
                const int di = std::abs(ii - i);
                for (int jj = j_lo; jj <= j_hi; jj++) {
                    if (std::max(di, std::abs(jj - j)) < d_min)
                        continue;
                    samples.push_back(row[jj]);
                }
            }

            std::sort(samples.begin(), samples.end());
            const size_t m = samples.size();
            /* Even count: mean of the two middle values */
            const int median = (m % 2) ? samples[m / 2]
                                       : (samples[m / 2 - 1] + samples[m / 2]) / 2;

            if (elev - median <= kSpikeAboveMedian)
                continue;

            dst[idx] = median;
            changed++;
            stats.max_excess = std::max(stats.max_excess, elev - median);
        }
    }

    return changed;
}

/* TODO: After the apex of a multi-cell spike is removed, lower cells in the
 * same blob can remain elevated above the ring (residual tail). A stronger
 * morphological filter may be needed if those tails still affect propagation. */
DespikeStats DespikeGrid(std::vector<int> &grid, int n) {
    DespikeStats stats;
    if (n < 3)
        return stats;

    for (int pass = 0; pass < kDespikeMaxPasses; pass++) {
        const std::vector<int> src(grid);
        const size_t changed = DespikePass(src, grid, n, stats);
        if (changed == 0)
            break;
        stats.removed += changed;
        stats.passes++;
    }

    return stats;
}

enum class PageResult { Written, Skipped };

PageResult GeneratePage(TileProvider &provider, const Config &cfg, int zoom,
                        const Page &page, const std::string &progress_tag) {
    const std::string name = PageName(page, cfg);
    const fs::path out_path = fs::path(cfg.outdir) / name;

    std::error_code ec;
    if (fs::exists(out_path, ec)) {
        LogOut(progress_tag + " " + name + ": exists, skipping");
        return PageResult::Skipped;
    }

    if (cfg.offline)
        CheckOfflineTiles(provider, page, cfg, zoom, name);

    const int ippd = cfg.hd ? 3600 : 1200;
    const int mpi = ippd - 1;
    const double ppd = ippd;
    const int64_t n_tiles = 1LL << zoom;
    const int64_t N = static_cast<int64_t>(kTileSize) * n_tiles;
    const double Nd = static_cast<double>(N);

    int max_west, min_west;
    WestRange(page.lon, max_west, min_west);

    LogOut(progress_tag + " " + name + ": generating (zoom " +
           std::to_string(zoom) + ")");

    /* Cache sized to hold two tile rows of the page width */
    TileRange tr = PageTileRange(page, cfg, zoom);
    size_t width = static_cast<size_t>(tr.x_hi - tr.x_lo + 1);
    TileCache cache(provider, zoom, 2 * (width + 2) + 4);

    /* Column geometry: j = 0 is the eastern edge (see FindMask) */
    std::vector<int64_t> col_x0(ippd), col_x1(ippd);
    std::vector<double> col_t(ippd);

    for (int j = 0; j < ippd; j++) {
        double lon_east = page.lon + (mpi - j) / ppd;
        double fx = MercatorX(lon_east) * Nd - 0.5;
        double fl = std::floor(fx);

        col_t[j] = fx - fl;
        col_x0[j] = WrapMod(static_cast<int64_t>(fl), N);
        col_x1[j] = WrapMod(static_cast<int64_t>(fl) + 1, N);
    }

    fs::create_directories(cfg.outdir, ec);
    fs::path tmp_path = out_path;
    tmp_path += ".part";

    try {
        /* Fill the whole page first so that it can be despiked before
         * anything is written. Row i runs south to north, column j runs
         * east to west (LoadSDF order). */
        std::vector<int> grid(static_cast<size_t>(ippd) * ippd);
        int next_report = 25;

        for (int i = 0; i < ippd; i++) {
            double lat = page.lat + i / ppd;
            double fy = MercatorY(lat) * Nd - 0.5;
            double fl = std::floor(fy);
            double ty = fy - fl;
            int64_t y0 = std::clamp<int64_t>(static_cast<int64_t>(fl), 0, N - 1);
            int64_t y1 =
                std::clamp<int64_t>(static_cast<int64_t>(fl) + 1, 0, N - 1);

            for (int j = 0; j < ippd; j++) {
                double tx = col_t[j];
                double v00 = cache.Pixel(col_x0[j], y0);
                double v10 = cache.Pixel(col_x1[j], y0);
                double v01 = cache.Pixel(col_x0[j], y1);
                double v11 = cache.Pixel(col_x1[j], y1);

                double v = (v00 * (1.0 - tx) + v10 * tx) * (1.0 - ty) +
                           (v01 * (1.0 - tx) + v11 * tx) * ty;

                grid[static_cast<size_t>(i) * ippd + j] =
                    static_cast<int>(std::floor(v + 0.5));
            }

            int pct = static_cast<int>((i + 1) * 100LL / ippd);
            if (pct >= next_report && pct < 100) {
                LogOut(progress_tag + " " + name + ": " + std::to_string(pct) +
                       "%");
                next_report += 25;
            }
        }

        if (cfg.despike) {
            DespikeStats ds = DespikeGrid(grid, ippd);
            if (ds.removed > 0)
                LogOut(progress_tag + " " + name + ": despike: removed " +
                       std::to_string(ds.removed) + " cell(s) in " +
                       std::to_string(ds.passes) + " pass(es) (max excess " +
                       std::to_string(ds.max_excess) + "m)");
        }

        OutputFile out(tmp_path, cfg.bz2);

        char header[128];
        int hlen = std::snprintf(header, sizeof(header), "%d\n%d\n%d\n%d\n",
                                 max_west, page.lat, min_west, page.lat + 1);
        out.Write(header, static_cast<size_t>(hlen));

        std::string buf;
        buf.reserve((1 << 20) + 64);

        for (int v : grid) {
            char num[16];
            auto res = std::to_chars(num, num + sizeof(num), v);
            buf.append(num, res.ptr);
            buf.push_back('\n');

            if (buf.size() >= (1 << 20)) {
                out.Write(buf.data(), buf.size());
                buf.clear();
            }
        }

        if (!buf.empty())
            out.Write(buf.data(), buf.size());

        out.Close();
    } catch (...) {
        fs::remove(tmp_path, ec);
        throw;
    }

    fs::rename(tmp_path, out_path, ec);
    if (ec) {
        fs::remove(tmp_path, ec);
        throw std::runtime_error("cannot move output into place: " +
                                 out_path.string());
    }

    const CacheStats &st = cache.Stats();
    LogOut(progress_tag + " " + name + ": done (" +
           std::to_string(st.tiles_fetched) + " tiles, " +
           std::to_string(st.bytes_fetched / 1024) + " KiB read)");

    if (st.tiles_missing > 0) {
        std::string msg = "Warning: " + name + ": " +
                          std::to_string(st.tiles_missing) +
                          " tile(s) not present in the source were treated as "
                          "sea level (0 m), e.g.";
        for (const std::string &t : st.missing_examples)
            msg += " " + t;
        LogErr(msg);
    }

    return PageResult::Written;
}

/* ------------------------------------------------------------------ */
/* Command line                                                       */
/* ------------------------------------------------------------------ */

void Usage(FILE *f, const char *prog) {
    std::fprintf(
        f,
        "\n%s: Generates SPLAT! elevation data files (.sdf) from Mapterhorn\n"
        "Terrarium WebP tiles stored in a PMTiles v3 archive.\n\n"
        "Usage:\n"
        "  %s [options] <min_north> <lon_east>\n"
        "  %s [options] --bbox minlat,minlon,maxlat,maxlon\n\n"
        "<min_north> and <lon_east> are the integer south-west corner of a\n"
        "1x1 degree page (longitude in degrees EAST, negative for west).\n"
        "A bounding box generates every page that overlaps it.\n\n"
        "Options:\n"
        "  --hd               High definition (3600 ppd); also enabled when the\n"
        "                     program name contains \"-hd\"\n"
        "  --zoom N           Tile zoom level (default 12 for HD, 10 otherwise)\n"
        "  --source PATH|URL  PMTiles archive (default %s)\n"
        "  --tiles-url URL    Fetch individual tiles from a URL template with\n"
        "                     {z}, {x}, {y} placeholders (for example\n"
        "                     https://tiles.mapterhorn.com/{z}/{x}/{y}.webp)\n"
        "                     instead of reading a PMTiles archive\n"
        "  --outdir DIR       Output directory (default .)\n"
        "  --bz2              Write .sdf.bz2 files\n"
        "  --offline          Refuse remote sources; for a local archive, fail\n"
        "                     and list any required tiles that are missing\n"
        "  --no-despike       Do not remove elevation spikes. By default a\n"
        "                     cell more than 100 m above every cell on a ring\n"
        "                     6 cells away and more than 150 m above the ring\n"
        "                     median is replaced by that median (up to 3\n"
        "                     passes, to remove multi-cell blobs)\n"
        "  --workers N        Parallel workers for --bbox (default 1)\n"
        "  --delimiter STR    Filename delimiter (default \"_\")\n"
        "  -h, --help         Show this help\n\n"
        "Existing output files are never overwritten.\n\n%s\n\n",
        prog, prog, prog, kDefaultSource, kAttribution);
}

bool ParseInt(const char *s, int &out) {
    char *end = nullptr;
    errno = 0;
    long v = std::strtol(s, &end, 10);
    if (end == s || *end != '\0' || errno != 0 || v < INT_MIN || v > INT_MAX)
        return false;
    out = static_cast<int>(v);
    return true;
}

bool ParseBBox(const char *s, double &minlat, double &minlon, double &maxlat,
               double &maxlon) {
    char tail = 0;
    return std::sscanf(s, "%lf,%lf,%lf,%lf%c", &minlat, &minlon, &maxlat,
                       &maxlon, &tail) == 4;
}

bool IsRemote(const std::string &s) {
    return s.rfind("http://", 0) == 0 || s.rfind("https://", 0) == 0;
}

} // namespace

int main(int argc, char **argv) {
    Config cfg;
    const char *prog = argc > 0 ? argv[0] : "mapterhorn2sdf";

    if (std::strstr(prog, "-hd") != nullptr)
        cfg.hd = true;

    bool have_bbox = false;
    double bb_minlat = 0, bb_minlon = 0, bb_maxlat = 0, bb_maxlon = 0;
    std::vector<std::string> positional;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        /* Negative numbers are positional, everything else with a dash
         * is an option. */
        bool is_option = arg.size() > 1 && arg[0] == '-' &&
                         !(std::isdigit(static_cast<unsigned char>(arg[1])));

        if (!is_option) {
            positional.push_back(arg);
            continue;
        }

        std::string value;
        bool has_inline = false;
        size_t eq = arg.find('=');
        if (arg.rfind("--", 0) == 0 && eq != std::string::npos) {
            value = arg.substr(eq + 1);
            arg = arg.substr(0, eq);
            has_inline = true;
        }

        auto need_value = [&](const char *opt) -> std::string {
            if (has_inline)
                return value;
            if (i + 1 >= argc) {
                std::fprintf(stderr, "*** Error: option %s requires a value\n",
                             opt);
                std::exit(2);
            }
            return argv[++i];
        };

        if (arg == "-h" || arg == "--help") {
            Usage(stdout, prog);
            return 0;
        } else if (arg == "--hd") {
            cfg.hd = true;
        } else if (arg == "--zoom") {
            std::string v = need_value("--zoom");
            if (!ParseInt(v.c_str(), cfg.zoom) || cfg.zoom < 1 ||
                cfg.zoom > 20) {
                std::fprintf(stderr,
                             "*** Error: --zoom must be an integer from 1 to "
                             "20\n");
                return 2;
            }
        } else if (arg == "--source") {
            cfg.source = need_value("--source");
        } else if (arg == "--tiles-url") {
            cfg.tiles_url = need_value("--tiles-url");
        } else if (arg == "--outdir") {
            cfg.outdir = need_value("--outdir");
        } else if (arg == "--bz2") {
            cfg.bz2 = true;
        } else if (arg == "--offline") {
            cfg.offline = true;
        } else if (arg == "--no-despike") {
            cfg.despike = false;
        } else if (arg == "--workers") {
            std::string v = need_value("--workers");
            if (!ParseInt(v.c_str(), cfg.workers) || cfg.workers < 1 ||
                cfg.workers > 64) {
                std::fprintf(stderr,
                             "*** Error: --workers must be an integer from 1 "
                             "to 64\n");
                return 2;
            }
        } else if (arg == "--delimiter") {
            cfg.delimiter = need_value("--delimiter");
        } else if (arg == "--bbox") {
            std::string v = need_value("--bbox");
            if (!ParseBBox(v.c_str(), bb_minlat, bb_minlon, bb_maxlat,
                           bb_maxlon)) {
                std::fprintf(stderr, "*** Error: --bbox expects "
                                     "minlat,minlon,maxlat,maxlon\n");
                return 2;
            }
            have_bbox = true;
        } else {
            std::fprintf(stderr, "*** Error: unknown option \"%s\"\n",
                         arg.c_str());
            Usage(stderr, prog);
            return 2;
        }
    }

    if (argc == 1) {
        Usage(stderr, prog);
        return 2;
    }

    /* Build the page list */
    std::vector<Page> pages;

    if (have_bbox) {
        if (!positional.empty()) {
            std::fprintf(stderr, "*** Error: --bbox cannot be combined with "
                                 "positional coordinates\n");
            return 2;
        }
        if (bb_minlat > bb_maxlat || bb_minlon > bb_maxlon) {
            std::fprintf(stderr, "*** Error: --bbox minimum exceeds maximum "
                                 "(boxes crossing the antimeridian are not "
                                 "supported)\n");
            return 2;
        }

        int lat0 = static_cast<int>(std::floor(bb_minlat));
        int lat1 = static_cast<int>(std::ceil(bb_maxlat)) - 1;
        int lon0 = static_cast<int>(std::floor(bb_minlon));
        int lon1 = static_cast<int>(std::ceil(bb_maxlon)) - 1;
        lat1 = std::max(lat0, lat1);
        lon1 = std::max(lon0, lon1);

        if (lat0 < -85 || lat1 > 84 || lon0 < -180 || lon1 > 179) {
            std::fprintf(stderr,
                         "*** Error: --bbox must lie within latitude -85..85 "
                         "and longitude -180..180\n");
            return 2;
        }

        for (int la = lat0; la <= lat1; la++)
            for (int lo = lon0; lo <= lon1; lo++)
                pages.push_back({la, lo});
    } else {
        int lat, lon;
        if (positional.size() != 2 || !ParseInt(positional[0].c_str(), lat) ||
            !ParseInt(positional[1].c_str(), lon)) {
            std::fprintf(stderr, "*** Error: expected integer <min_north> and "
                                 "<lon_east> arguments, or --bbox\n");
            Usage(stderr, prog);
            return 2;
        }
        if (lat < -85 || lat > 84 || lon < -180 || lon > 179) {
            std::fprintf(stderr,
                         "*** Error: min_north must be -85..84 and lon_east "
                         "must be -180..179\n");
            return 2;
        }
        pages.push_back({lat, lon});
    }

    if (cfg.zoom < 0)
        cfg.zoom = cfg.hd ? 12 : 10;

    const bool use_direct = !cfg.tiles_url.empty();

    if (cfg.offline) {
        if (use_direct) {
            std::fprintf(stderr, "*** Error: --offline cannot be combined "
                                 "with --tiles-url\n");
            return 2;
        }
        if (IsRemote(cfg.source)) {
            std::fprintf(stderr,
                         "*** Error: --offline requires a local PMTiles file, "
                         "but the source is a remote URL: %s\n",
                         cfg.source.c_str());
            return 2;
        }
    }

    std::printf("%s\n", kAttribution);
    std::fflush(stdout);

    curl_global_init(CURL_GLOBAL_DEFAULT);

    /* Open the tile source */
    std::unique_ptr<TileProvider> provider;

    try {
        if (use_direct) {
            provider.reset(new DirectUrlProvider(cfg.tiles_url));
        } else {
            std::unique_ptr<ByteSource> src;
            if (IsRemote(cfg.source))
                src.reset(new HttpSource(cfg.source));
            else
                src.reset(new LocalSource(cfg.source));
            provider.reset(new PMTilesProvider(std::move(src)));
        }

        int min_z, max_z;
        if (provider->ZoomRange(min_z, max_z) &&
            (cfg.zoom < min_z || cfg.zoom > max_z)) {
            std::fprintf(stderr,
                         "*** Error: zoom %d is outside the archive range "
                         "%d..%d\n",
                         cfg.zoom, min_z, max_z);
            return 1;
        }
    } catch (const std::exception &e) {
        std::fprintf(stderr, "*** Error: %s\n", e.what());
        return 1;
    }

    const int workers =
        std::min<int>(cfg.workers, static_cast<int>(pages.size()));

    std::printf("Mode: %s, zoom %d, %zu page(s), %d worker(s)\n",
                cfg.hd ? "HD (3600 ppd)" : "standard (1200 ppd)", cfg.zoom,
                pages.size(), workers);
    std::fflush(stdout);

    std::atomic<size_t> next_page(0);
    std::atomic<int> written(0), skipped(0), failed(0);

    auto worker = [&]() {
        for (;;) {
            size_t idx = next_page.fetch_add(1);
            if (idx >= pages.size())
                return;

            std::string tag = "[" + std::to_string(idx + 1) + "/" +
                              std::to_string(pages.size()) + "]";

            try {
                PageResult r = GeneratePage(*provider, cfg, cfg.zoom,
                                            pages[idx], tag);
                if (r == PageResult::Written)
                    written++;
                else
                    skipped++;
            } catch (const std::exception &e) {
                failed++;
                LogErr("*** Error: " + tag + " " + PageName(pages[idx], cfg) +
                       ": " + e.what());
            }
        }
    };

    std::vector<std::thread> threads;
    for (int t = 1; t < workers; t++)
        threads.emplace_back(worker);
    worker();
    for (std::thread &t : threads)
        t.join();

    LogOut("Finished: " + std::to_string(written.load()) + " written, " +
           std::to_string(skipped.load()) + " skipped, " +
           std::to_string(failed.load()) + " failed");

    return failed.load() > 0 ? 1 : 0;
}
