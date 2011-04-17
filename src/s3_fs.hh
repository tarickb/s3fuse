#ifndef S3_FS_HH
#define S3_FS_HH

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <time.h>

#include <map>
#include <string>
#include <pugixml/pugixml.hpp>

namespace s3
{
  class fs
  {
  public:
    fs(const std::string &bucket);
    ~fs();

    int get_stats(const std::string &path, struct stat *s);
    int read_directory(const std::string &path, fuse_fill_dir_t filler, void *buf);

  private:
    struct file_stats
    {
      time_t expiry;
      struct stat stats;

      file_stats() : expiry(0) {}
    };

    typedef std::map<std::string, file_stats> stats_map;

    bool get_cached_stats(const std::string &path, struct stat *s);
    void update_stats_cache(const std::string &path, const struct stat *s);

    std::string _bucket;
    stats_map _stats_map;  
    pugi::xpath_query _prefix_query;
    pugi::xpath_query _key_query;
  };
}

#endif
