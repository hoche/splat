/*
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */
#define __STDC_WANT_LIB_EXT1__ 1

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

// Use zlib instead of miniz
#include <zlib.h>

#if defined(_WIN32) || defined(__WIN32__) || defined(_MSC_VER) ||              \
    defined(__MINGW32__)
/* Win32, DOS, MSVC, MSVS */
#include <direct.h>

#define MKDIR(DIRNAME) _mkdir(DIRNAME)
#define STRCLONE(STR) ((STR) ? _strdup(STR) : NULL)
#define HAS_DEVICE(P)                                                          \
  ((((P)[0] >= 'A' && (P)[0] <= 'Z') || ((P)[0] >= 'a' && (P)[0] <= 'z')) &&   \
   (P)[1] == ':')
#define FILESYSTEM_PREFIX_LEN(P) (HAS_DEVICE(P) ? 2 : 0)

#else

#include <unistd.h> // needed for symlink() on BSD
int symlink(const char *target, const char *linkpath); // needed on Linux

#define MKDIR(DIRNAME) mkdir(DIRNAME, 0755)
#define STRCLONE(STR) ((STR) ? strdup(STR) : NULL)

#endif

#include "zip.h"

#ifndef HAS_DEVICE
#define HAS_DEVICE(P) 0
#endif

#ifndef FILESYSTEM_PREFIX_LEN
#define FILESYSTEM_PREFIX_LEN(P) 0
#endif

#ifndef ISSLASH
#define ISSLASH(C) ((C) == '/' || (C) == '\\')
#endif

#define CLEANUP(ptr)                                                           \
  do {                                                                         \
    if (ptr) {                                                                 \
      free((void *)ptr);                                                       \
      ptr = NULL;                                                              \
    }                                                                          \
  } while (0)

// ZIP file format constants
#define ZIP_LOCAL_FILE_HEADER_SIG 0x04034b50
#define ZIP_CENTRAL_DIR_HEADER_SIG 0x02014b50
#define ZIP_END_OF_CENTRAL_DIR_SIG 0x06054b50
#define ZIP_LOCAL_FILE_HEADER_SIZE 30
#define ZIP_CENTRAL_DIR_HEADER_SIZE 46
#define ZIP_END_OF_CENTRAL_DIR_SIZE 22

// ZIP compression methods
#define ZIP_COMP_STORE 0
#define ZIP_COMP_DEFLATE 8

// Buffer size for compression
#define BUFFER_SIZE 8192

// Helper functions
static void write_le16(unsigned char *buf, unsigned short val) {
  buf[0] = val & 0xFF;
  buf[1] = (val >> 8) & 0xFF;
}

static void write_le32(unsigned char *buf, unsigned int val) {
  buf[0] = val & 0xFF;
  buf[1] = (val >> 8) & 0xFF;
  buf[2] = (val >> 16) & 0xFF;
  buf[3] = (val >> 24) & 0xFF;
}

static unsigned short read_le16(const unsigned char *buf) {
  return buf[0] | (buf[1] << 8);
}

static unsigned int read_le32(const unsigned char *buf) {
  return buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
}

static const char *base_name(const char *name) {
  char const *p;
  char const *base = name += FILESYSTEM_PREFIX_LEN(name);
  int all_slashes = 1;

  for (p = name; *p; p++) {
    if (ISSLASH(*p))
      base = p + 1;
    else
      all_slashes = 0;
  }

  /* If NAME is all slashes, arrange to return `/'. */
  if (*base == '\0' && ISSLASH(*name) && all_slashes)
    --base;

  return base;
}

static int mkpath(char *path) {
  char *p;
  char npath[MAX_PATH + 1];
  int len = 0;
  int has_device = HAS_DEVICE(path);

  memset(npath, 0, MAX_PATH + 1);
  if (has_device) {
    // only on windows
    npath[0] = path[0];
    npath[1] = path[1];
    len = 2;
  }
  for (p = path + len; *p && len < MAX_PATH; p++) {
    if (ISSLASH(*p) && ((!has_device && len > 0) || (has_device && len > 2))) {
#if defined(_WIN32) || defined(__WIN32__) || defined(_MSC_VER) ||              \
    defined(__MINGW32__)
#else
      if ('\\' == *p) {
        *p = '/';
      }
#endif

      if (MKDIR(npath) == -1) {
        if (errno != EEXIST) {
          return -1;
        }
      }
    }
    npath[len++] = *p;
  }

  return 0;
}

static char *strrpl(const char *str, size_t n, char oldchar, char newchar) {
  char c;
  size_t i;
  char *rpl = (char *)calloc((1 + n), sizeof(char));
  char *begin = rpl;
  if (!rpl) {
    return NULL;
  }

  for (i = 0; (i < n) && (c = *str++); ++i) {
    if (c == oldchar) {
      c = newchar;
    }
    *rpl++ = c;
  }

  return begin;
}

static void dos_date_time(time_t t, unsigned short *dos_date, unsigned short *dos_time) {
  struct tm *tm_time = localtime(&t);
  if (tm_time) {
    *dos_date = ((tm_time->tm_year - 80) << 9) | 
                ((tm_time->tm_mon + 1) << 5) | 
                tm_time->tm_mday;
    *dos_time = (tm_time->tm_hour << 11) | 
                (tm_time->tm_min << 5) | 
                (tm_time->tm_sec / 2);
  } else {
    *dos_date = 0;
    *dos_time = 0;
  }
}

// Central directory entry structure
typedef struct {
  char *filename;
  unsigned int crc32;
  unsigned int compressed_size;
  unsigned int uncompressed_size;
  unsigned int local_header_offset;
  unsigned int file_data_offset;
  unsigned short dos_time;
  unsigned short dos_date;
  unsigned short compression_method;
  unsigned int external_attr;
} zip_cd_entry;

struct zip_entry_t {
  int index;
  char *name;
  unsigned long long uncomp_size;
  unsigned long long comp_size;
  unsigned int uncomp_crc32;
  unsigned long long offset;
  unsigned long long header_offset;
  unsigned short method;
  unsigned int external_attr;
  time_t m_time;
  z_stream stream;
  int stream_initialized;
};

struct zip_t {
  FILE *fp;
  int level;
  char mode;
  char *filename;
  struct zip_entry_t entry;
  zip_cd_entry *cd_entries;
  int num_entries;
  int cd_capacity;
};

struct zip_t *zip_open(const char *zipname, int level, char mode) {
  struct zip_t *zip = NULL;

  if (!zipname || strlen(zipname) < 1) {
    goto cleanup;
  }

  if (level < 0)
    level = ZIP_DEFAULT_COMPRESSION_LEVEL;
  if (level > 9) {
    goto cleanup;
  }

  zip = (struct zip_t *)calloc((size_t)1, sizeof(struct zip_t));
  if (!zip)
    goto cleanup;

  zip->level = level;
  zip->mode = mode;
  
  const char *fmode = NULL;
  switch (mode) {
  case 'w':
    fmode = "wb";
    break;
  case 'r':
    fmode = "rb";
    break;
  case 'a':
    fmode = "r+b";
    break;
  default:
    goto cleanup;
  }

  zip->fp = fopen(zipname, fmode);
  if (!zip->fp) {
    if (mode == 'a') {
      // Try creating if append mode and file doesn't exist
      zip->fp = fopen(zipname, "w+b");
    }
    if (!zip->fp) {
      goto cleanup;
    }
  }

  zip->filename = strdup(zipname);
  if (!zip->filename) {
    goto cleanup;
  }

  // For read or append mode, read the central directory
  if (mode == 'r' || mode == 'a') {
    // Find end of central directory record
    fseek(zip->fp, 0, SEEK_END);
    long file_size = ftell(zip->fp);
    
    // Search for EOCD signature (scan last 65KB + EOCD size)
    long search_start = file_size - 65536 - ZIP_END_OF_CENTRAL_DIR_SIZE;
    if (search_start < 0) search_start = 0;
    
    unsigned char buffer[ZIP_END_OF_CENTRAL_DIR_SIZE];
    int found = 0;
    long eocd_offset = 0;
    
    for (long pos = file_size - ZIP_END_OF_CENTRAL_DIR_SIZE; pos >= search_start; pos--) {
      fseek(zip->fp, pos, SEEK_SET);
      if (fread(buffer, 1, 4, zip->fp) != 4) break;
      
      if (read_le32(buffer) == ZIP_END_OF_CENTRAL_DIR_SIG) {
        eocd_offset = pos;
        found = 1;
        break;
      }
    }
    
    if (!found) {
      goto cleanup;
    }
    
    // Read EOCD record
    fseek(zip->fp, eocd_offset, SEEK_SET);
    if (fread(buffer, 1, ZIP_END_OF_CENTRAL_DIR_SIZE, zip->fp) != ZIP_END_OF_CENTRAL_DIR_SIZE) {
      goto cleanup;
    }
    
    int num_entries = read_le16(&buffer[8]);
    unsigned int cd_size = read_le32(&buffer[12]);
    unsigned int cd_offset = read_le32(&buffer[16]);
    
    // Read central directory
    zip->cd_entries = malloc(num_entries * sizeof(zip_cd_entry));
    if (!zip->cd_entries) {
      goto cleanup;
    }
    zip->cd_capacity = num_entries;
    
    fseek(zip->fp, cd_offset, SEEK_SET);
    
    for (int i = 0; i < num_entries; i++) {
      unsigned char cd_header[ZIP_CENTRAL_DIR_HEADER_SIZE];
      if (fread(cd_header, 1, ZIP_CENTRAL_DIR_HEADER_SIZE, zip->fp) != ZIP_CENTRAL_DIR_HEADER_SIZE) {
        goto cleanup;
      }
      
      if (read_le32(&cd_header[0]) != ZIP_CENTRAL_DIR_HEADER_SIG) {
        goto cleanup;
      }
      
      zip_cd_entry *entry = &zip->cd_entries[i];
      entry->compression_method = read_le16(&cd_header[10]);
      entry->dos_time = read_le16(&cd_header[12]);
      entry->dos_date = read_le16(&cd_header[14]);
      entry->crc32 = read_le32(&cd_header[16]);
      entry->compressed_size = read_le32(&cd_header[20]);
      entry->uncompressed_size = read_le32(&cd_header[24]);
      unsigned short filename_len = read_le16(&cd_header[28]);
      unsigned short extra_len = read_le16(&cd_header[30]);
      unsigned short comment_len = read_le16(&cd_header[32]);
      entry->external_attr = read_le32(&cd_header[38]);
      entry->local_header_offset = read_le32(&cd_header[42]);
      
      // Read filename
      entry->filename = malloc(filename_len + 1);
      if (!entry->filename) {
        goto cleanup;
      }
      if (fread(entry->filename, 1, filename_len, zip->fp) != filename_len) {
        free(entry->filename);
        goto cleanup;
      }
      entry->filename[filename_len] = '\0';
      
      // Skip extra field and comment
      fseek(zip->fp, extra_len + comment_len, SEEK_CUR);
      
      // Find actual file data offset by reading local header
      long current_pos = ftell(zip->fp);
      fseek(zip->fp, entry->local_header_offset, SEEK_SET);
      
      unsigned char local_header[ZIP_LOCAL_FILE_HEADER_SIZE];
      if (fread(local_header, 1, ZIP_LOCAL_FILE_HEADER_SIZE, zip->fp) != ZIP_LOCAL_FILE_HEADER_SIZE) {
        goto cleanup;
      }
      
      unsigned short local_filename_len = read_le16(&local_header[26]);
      unsigned short local_extra_len = read_le16(&local_header[28]);
      entry->file_data_offset = entry->local_header_offset + ZIP_LOCAL_FILE_HEADER_SIZE + 
                                local_filename_len + local_extra_len;
      
      fseek(zip->fp, current_pos, SEEK_SET);
    }
    
    zip->num_entries = num_entries;
  }

  return zip;

cleanup:
  if (zip) {
    if (zip->fp) {
      fclose(zip->fp);
    }
    CLEANUP(zip);
  }
  return NULL;
}

void zip_close(struct zip_t *zip) {
  if (zip) {
    // Clean up current entry if open
    if (zip->entry.name) {
      CLEANUP(zip->entry.name);
    }
    if (zip->entry.stream_initialized) {
      deflateEnd(&zip->entry.stream);
    }
    
    // Write central directory and end of central directory record
    if (zip->mode == 'w' || zip->mode == 'a') {
      unsigned long central_dir_offset = ftell(zip->fp);
      unsigned long central_dir_size = 0;

      // Write central directory entries
      for (int i = 0; i < zip->num_entries; i++) {
        zip_cd_entry *entry = &zip->cd_entries[i];
        unsigned short filename_len = strlen(entry->filename);
        
        unsigned char cd_header[ZIP_CENTRAL_DIR_HEADER_SIZE];
        memset(cd_header, 0, sizeof(cd_header));
        
        write_le32(&cd_header[0], ZIP_CENTRAL_DIR_HEADER_SIG);
        write_le16(&cd_header[4], 0x031E); // version made by (3.0, Unix)
        write_le16(&cd_header[6], 20); // version needed to extract
        write_le16(&cd_header[8], 0); // general purpose bit flag
        write_le16(&cd_header[10], entry->compression_method);
        write_le16(&cd_header[12], entry->dos_time);
        write_le16(&cd_header[14], entry->dos_date);
        write_le32(&cd_header[16], entry->crc32);
        write_le32(&cd_header[20], entry->compressed_size);
        write_le32(&cd_header[24], entry->uncompressed_size);
        write_le16(&cd_header[28], filename_len);
        write_le16(&cd_header[30], 0); // extra field length
        write_le16(&cd_header[32], 0); // file comment length
        write_le16(&cd_header[34], 0); // disk number start
        write_le16(&cd_header[36], 0); // internal file attributes
        write_le32(&cd_header[38], entry->external_attr);
        write_le32(&cd_header[42], entry->local_header_offset);
        
        fwrite(cd_header, 1, ZIP_CENTRAL_DIR_HEADER_SIZE, zip->fp);
        fwrite(entry->filename, 1, filename_len, zip->fp);
        
        central_dir_size += ZIP_CENTRAL_DIR_HEADER_SIZE + filename_len;
      }

      // Write end of central directory record
      unsigned char eocd[ZIP_END_OF_CENTRAL_DIR_SIZE];
      memset(eocd, 0, sizeof(eocd));
      
      write_le32(&eocd[0], ZIP_END_OF_CENTRAL_DIR_SIG);
      write_le16(&eocd[4], 0); // disk number
      write_le16(&eocd[6], 0); // disk number with central dir
      write_le16(&eocd[8], zip->num_entries); // num entries this disk
      write_le16(&eocd[10], zip->num_entries); // total num entries
      write_le32(&eocd[12], central_dir_size);
      write_le32(&eocd[16], central_dir_offset);
      write_le16(&eocd[20], 0); // comment length
      
      fwrite(eocd, 1, ZIP_END_OF_CENTRAL_DIR_SIZE, zip->fp);
    }

    if (zip->fp) {
      fclose(zip->fp);
    }

    // Free central directory entries
    for (int i = 0; i < zip->num_entries; i++) {
      CLEANUP(zip->cd_entries[i].filename);
    }
    CLEANUP(zip->cd_entries);
    CLEANUP(zip->filename);

    CLEANUP(zip);
  }
}

int zip_entry_open(struct zip_t *zip, const char *entryname) {
  size_t entrylen = 0;

  if (!zip || !entryname) {
    return -1;
  }

  entrylen = strlen(entryname);
  if (entrylen < 1) {
    return -1;
  }

  zip->entry.name = strrpl(entryname, entrylen, '\\', '/');
  if (!zip->entry.name) {
    return -1;
  }

  if (zip->mode == 'r') {
    // Search for entry in central directory
    for (int i = 0; i < zip->num_entries; i++) {
      if (strcmp(zip->cd_entries[i].filename, zip->entry.name) == 0) {
        CLEANUP(zip->entry.name);
        return zip_entry_openbyindex(zip, i);
      }
    }
    CLEANUP(zip->entry.name);
    return -1;
  }

  // Writing mode - prepare for new entry
  zip->entry.index = zip->num_entries;
  zip->entry.uncomp_size = 0;
  zip->entry.comp_size = 0;
  zip->entry.uncomp_crc32 = crc32(0L, Z_NULL, 0);
  zip->entry.method = (zip->level > 0) ? ZIP_COMP_DEFLATE : ZIP_COMP_STORE;
  zip->entry.m_time = time(NULL);
  zip->entry.external_attr = (0100644) << 16; // regular file, rw-r--r--
  zip->entry.stream_initialized = 0;

  // Remember position for local file header
  zip->entry.header_offset = ftell(zip->fp);

  // Write placeholder local file header (will update later)
  unsigned char local_header[ZIP_LOCAL_FILE_HEADER_SIZE];
  memset(local_header, 0, sizeof(local_header));
  
  unsigned short filename_len = strlen(zip->entry.name);
  
  write_le32(&local_header[0], ZIP_LOCAL_FILE_HEADER_SIG);
  write_le16(&local_header[4], 20); // version needed to extract
  write_le16(&local_header[6], 0); // general purpose bit flag
  write_le16(&local_header[8], zip->entry.method);
  // dos_time and dos_date will be filled in at close
  write_le16(&local_header[26], filename_len);
  write_le16(&local_header[28], 0); // extra field length
  
  fwrite(local_header, 1, ZIP_LOCAL_FILE_HEADER_SIZE, zip->fp);
  fwrite(zip->entry.name, 1, filename_len, zip->fp);

  zip->entry.offset = ftell(zip->fp);

  // Initialize compression stream if using deflate
  if (zip->entry.method == ZIP_COMP_DEFLATE) {
    zip->entry.stream.zalloc = Z_NULL;
    zip->entry.stream.zfree = Z_NULL;
    zip->entry.stream.opaque = Z_NULL;
    
    // Use deflateInit2 with negative window bits for raw deflate
    int ret = deflateInit2(&zip->entry.stream, zip->level, Z_DEFLATED,
                          -15, 8, Z_DEFAULT_STRATEGY);
    if (ret != Z_OK) {
      CLEANUP(zip->entry.name);
      return -1;
    }
    zip->entry.stream_initialized = 1;
  }

  return 0;
}

int zip_entry_openbyindex(struct zip_t *zip, int index) {
  if (!zip || zip->mode != 'r') {
    return -1;
  }
  
  if (index < 0 || index >= zip->num_entries) {
    return -1;
  }
  
  zip_cd_entry *cd_entry = &zip->cd_entries[index];
  
  zip->entry.index = index;
  zip->entry.name = strdup(cd_entry->filename);
  if (!zip->entry.name) {
    return -1;
  }
  
  zip->entry.uncomp_size = cd_entry->uncompressed_size;
  zip->entry.comp_size = cd_entry->compressed_size;
  zip->entry.uncomp_crc32 = cd_entry->crc32;
  zip->entry.method = cd_entry->compression_method;
  zip->entry.external_attr = cd_entry->external_attr;
  zip->entry.offset = cd_entry->file_data_offset;
  zip->entry.stream_initialized = 0;
  
  return 0;
}

int zip_entry_close(struct zip_t *zip) {
  if (!zip) {
    return -1;
  }

  if (zip->mode == 'r') {
    CLEANUP(zip->entry.name);
    return 0;
  }

  // Finish compression if needed
  if (zip->entry.stream_initialized) {
    unsigned char out_buffer[BUFFER_SIZE];
    int ret;
    
    zip->entry.stream.avail_in = 0;
    zip->entry.stream.next_in = Z_NULL;
    
    do {
      zip->entry.stream.avail_out = BUFFER_SIZE;
      zip->entry.stream.next_out = out_buffer;
      
      ret = deflate(&zip->entry.stream, Z_FINISH);
      
      int have = BUFFER_SIZE - zip->entry.stream.avail_out;
      if (have > 0) {
        fwrite(out_buffer, 1, have, zip->fp);
        zip->entry.comp_size += have;
      }
    } while (ret == Z_OK);
    
    deflateEnd(&zip->entry.stream);
    zip->entry.stream_initialized = 0;
    
    if (ret != Z_STREAM_END) {
      CLEANUP(zip->entry.name);
      return -1;
    }
  }

  // Update local file header
  unsigned long current_pos = ftell(zip->fp);
  fseek(zip->fp, zip->entry.header_offset, SEEK_SET);
  
  unsigned char local_header[ZIP_LOCAL_FILE_HEADER_SIZE];
  unsigned short dos_time, dos_date;
  dos_date_time(zip->entry.m_time, &dos_date, &dos_time);
  
  unsigned short filename_len = strlen(zip->entry.name);
  
  write_le32(&local_header[0], ZIP_LOCAL_FILE_HEADER_SIG);
  write_le16(&local_header[4], 20); // version needed to extract
  write_le16(&local_header[6], 0); // general purpose bit flag
  write_le16(&local_header[8], zip->entry.method);
  write_le16(&local_header[10], dos_time);
  write_le16(&local_header[12], dos_date);
  write_le32(&local_header[14], zip->entry.uncomp_crc32);
  write_le32(&local_header[18], zip->entry.comp_size);
  write_le32(&local_header[22], zip->entry.uncomp_size);
  write_le16(&local_header[26], filename_len);
  write_le16(&local_header[28], 0); // extra field length
  
  fwrite(local_header, 1, ZIP_LOCAL_FILE_HEADER_SIZE, zip->fp);
  
  fseek(zip->fp, current_pos, SEEK_SET);

  // Add to central directory list
  if (zip->num_entries >= zip->cd_capacity) {
    int new_capacity = zip->cd_capacity == 0 ? 16 : zip->cd_capacity * 2;
    zip_cd_entry *new_entries = realloc(zip->cd_entries, 
                                        new_capacity * sizeof(zip_cd_entry));
    if (!new_entries) {
      CLEANUP(zip->entry.name);
      return -1;
    }
    zip->cd_entries = new_entries;
    zip->cd_capacity = new_capacity;
  }

  zip_cd_entry *entry = &zip->cd_entries[zip->num_entries];
  entry->filename = zip->entry.name;
  entry->crc32 = zip->entry.uncomp_crc32;
  entry->compressed_size = zip->entry.comp_size;
  entry->uncompressed_size = zip->entry.uncomp_size;
  entry->local_header_offset = zip->entry.header_offset;
  entry->dos_time = dos_time;
  entry->dos_date = dos_date;
  entry->compression_method = zip->entry.method;
  entry->external_attr = zip->entry.external_attr;
  
  zip->num_entries++;
  zip->entry.name = NULL; // Transferred ownership to cd_entries

  return 0;
}

const char *zip_entry_name(struct zip_t *zip) {
  if (!zip) {
    return NULL;
  }
  return zip->entry.name;
}

int zip_entry_index(struct zip_t *zip) {
  if (!zip) {
    return -1;
  }
  return zip->entry.index;
}

int zip_entry_isdir(struct zip_t *zip) {
  if (!zip || zip->entry.index < 0) {
    return -1;
  }
  
  if (zip->entry.name) {
    size_t len = strlen(zip->entry.name);
    if (len > 0 && zip->entry.name[len - 1] == '/') {
      return 1;
    }
  }
  return 0;
}

unsigned long long zip_entry_size(struct zip_t *zip) {
  return zip ? zip->entry.uncomp_size : 0;
}

unsigned int zip_entry_crc32(struct zip_t *zip) {
  return zip ? zip->entry.uncomp_crc32 : 0;
}

int zip_entry_write(struct zip_t *zip, const void *buf, size_t bufsize) {
  if (!zip || zip->mode == 'r') {
    return -1;
  }

  if (!buf || bufsize == 0) {
    return 0;
  }

  // Update CRC32
  zip->entry.uncomp_crc32 = crc32(zip->entry.uncomp_crc32, buf, bufsize);
  zip->entry.uncomp_size += bufsize;

  if (zip->entry.method == ZIP_COMP_STORE) {
    // No compression
    fwrite(buf, 1, bufsize, zip->fp);
    zip->entry.comp_size += bufsize;
  } else {
    // Deflate compression
    unsigned char out_buffer[BUFFER_SIZE];
    
    zip->entry.stream.avail_in = bufsize;
    zip->entry.stream.next_in = (unsigned char *)buf;
    
    do {
      zip->entry.stream.avail_out = BUFFER_SIZE;
      zip->entry.stream.next_out = out_buffer;
      
      int ret = deflate(&zip->entry.stream, Z_NO_FLUSH);
      if (ret == Z_STREAM_ERROR) {
        return -1;
      }
      
      int have = BUFFER_SIZE - zip->entry.stream.avail_out;
      if (have > 0) {
        fwrite(out_buffer, 1, have, zip->fp);
        zip->entry.comp_size += have;
      }
    } while (zip->entry.stream.avail_out == 0);
  }

  return 0;
}

int zip_entry_fwrite(struct zip_t *zip, const char *filename) {
  if (!zip || zip->mode == 'r') {
    return -1;
  }

  FILE *in = fopen(filename, "rb");
  if (!in) {
    return -1;
  }

  // Get file stats
  struct stat st;
  if (stat(filename, &st) == 0) {
    zip->entry.m_time = st.st_mtime;
    if ((st.st_mode & 0200) == 0) {
      zip->entry.external_attr |= 0x01; // read-only
    }
    zip->entry.external_attr |= (st.st_mode & 0xFFFF) << 16;
  }

  unsigned char buffer[BUFFER_SIZE];
  size_t n;
  int ret = 0;

  while ((n = fread(buffer, 1, BUFFER_SIZE, in)) > 0) {
    if (zip_entry_write(zip, buffer, n) < 0) {
      ret = -1;
      break;
    }
  }

  fclose(in);
  return ret;
}

ssize_t zip_entry_read(struct zip_t *zip, void **buf, size_t *bufsize) {
  if (!zip || zip->mode != 'r' || zip->entry.index < 0) {
    return -1;
  }
  
  // Check if it's a directory
  if (zip_entry_isdir(zip)) {
    return -1;
  }
  
  // Allocate buffer for uncompressed data
  *buf = malloc(zip->entry.uncomp_size);
  if (!*buf) {
    return -1;
  }
  
  // Seek to file data
  fseek(zip->fp, zip->entry.offset, SEEK_SET);
  
  if (zip->entry.method == ZIP_COMP_STORE) {
    // No compression - read directly
    size_t bytes_read = fread(*buf, 1, zip->entry.uncomp_size, zip->fp);
    if (bytes_read != zip->entry.uncomp_size) {
      free(*buf);
      *buf = NULL;
      return -1;
    }
    if (bufsize) {
      *bufsize = bytes_read;
    }
    return bytes_read;
  } else if (zip->entry.method == ZIP_COMP_DEFLATE) {
    // Deflate compression - decompress
    unsigned char *comp_data = malloc(zip->entry.comp_size);
    if (!comp_data) {
      free(*buf);
      *buf = NULL;
      return -1;
    }
    
    if (fread(comp_data, 1, zip->entry.comp_size, zip->fp) != zip->entry.comp_size) {
      free(comp_data);
      free(*buf);
      *buf = NULL;
      return -1;
    }
    
    // Initialize inflate
    z_stream stream;
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
    stream.avail_in = zip->entry.comp_size;
    stream.next_in = comp_data;
    stream.avail_out = zip->entry.uncomp_size;
    stream.next_out = *buf;
    
    // Use inflateInit2 with negative window bits for raw deflate
    int ret = inflateInit2(&stream, -15);
    if (ret != Z_OK) {
      free(comp_data);
      free(*buf);
      *buf = NULL;
      return -1;
    }
    
    ret = inflate(&stream, Z_FINISH);
    inflateEnd(&stream);
    free(comp_data);
    
    if (ret != Z_STREAM_END) {
      free(*buf);
      *buf = NULL;
      return -1;
    }
    
    if (bufsize) {
      *bufsize = stream.total_out;
    }
    return stream.total_out;
  }
  
  free(*buf);
  *buf = NULL;
  return -1;
}

ssize_t zip_entry_noallocread(struct zip_t *zip, void *buf, size_t bufsize) {
  if (!zip || zip->mode != 'r' || zip->entry.index < 0) {
    return -1;
  }
  
  if (!buf) {
    return -1;
  }
  
  if (bufsize < zip->entry.uncomp_size) {
    return -1;
  }
  
  // Seek to file data
  fseek(zip->fp, zip->entry.offset, SEEK_SET);
  
  if (zip->entry.method == ZIP_COMP_STORE) {
    // No compression - read directly
    size_t bytes_read = fread(buf, 1, zip->entry.uncomp_size, zip->fp);
    if (bytes_read != zip->entry.uncomp_size) {
      return -1;
    }
    return bytes_read;
  } else if (zip->entry.method == ZIP_COMP_DEFLATE) {
    // Deflate compression - decompress
    unsigned char *comp_data = malloc(zip->entry.comp_size);
    if (!comp_data) {
      return -1;
    }
    
    if (fread(comp_data, 1, zip->entry.comp_size, zip->fp) != zip->entry.comp_size) {
      free(comp_data);
      return -1;
    }
    
    // Initialize inflate
    z_stream stream;
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
    stream.avail_in = zip->entry.comp_size;
    stream.next_in = comp_data;
    stream.avail_out = bufsize;
    stream.next_out = buf;
    
    // Use inflateInit2 with negative window bits for raw deflate
    int ret = inflateInit2(&stream, -15);
    if (ret != Z_OK) {
      free(comp_data);
      return -1;
    }
    
    ret = inflate(&stream, Z_FINISH);
    inflateEnd(&stream);
    free(comp_data);
    
    if (ret != Z_STREAM_END) {
      return -1;
    }
    
    return stream.total_out;
  }
  
  return -1;
}

int zip_entry_fread(struct zip_t *zip, const char *filename) {
  // Read mode not fully implemented
  if (!zip || zip->mode != 'r') {
    return -1;
  }
  return -1;
}

int zip_entry_extract(struct zip_t *zip,
                      size_t (*on_extract)(void *arg, unsigned long long offset,
                                           const void *buf, size_t bufsize),
                      void *arg) {
  // Read mode not fully implemented
  if (!zip || zip->mode != 'r') {
    return -1;
  }
  return -1;
}

int zip_total_entries(struct zip_t *zip) {
  if (!zip) {
    return -1;
  }
  return zip->num_entries;
}

int zip_create(const char *zipname, const char *filenames[], size_t len) {
  struct zip_t *zip = zip_open(zipname, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');
  if (!zip) {
    return -1;
  }

  int status = 0;
  for (size_t i = 0; i < len; i++) {
    const char *name = filenames[i];
    if (!name) {
      status = -1;
      break;
    }

    if (zip_entry_open(zip, base_name(name)) < 0) {
      status = -1;
      break;
    }

    if (zip_entry_fwrite(zip, name) < 0) {
      zip_entry_close(zip);
      status = -1;
      break;
    }

    if (zip_entry_close(zip) < 0) {
      status = -1;
      break;
    }
  }

  zip_close(zip);
  return status;
}

int zip_extract(const char *zipname, const char *dir,
                int (*on_extract)(const char *filename, void *arg), void *arg) {
  // Extract mode not fully implemented
  return -1;
}
