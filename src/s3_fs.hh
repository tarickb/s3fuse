#ifndef S3_FS_HH
#define S3_FS_HH

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <time.h>

#include <map>
#include <string>
#include <boost/thread.hpp>
#include <pugixml/pugixml.hpp>

#include "s3_async_queue.hh"

namespace s3
{
  class fs
  {
  public:
    fs(const std::string &bucket);
    ~fs();

    inline int get_stats(const std::string &path, struct stat *s)
    {
      return get_stats(path, s, HINT_NONE);
    }

    int read_directory(const std::string &path, fuse_fill_dir_t filler, void *buf);
    int create_object(const std::string &path, mode_t mode);

  private:
    struct file_stats
    {
      time_t expiry;
      struct stat stats;

      file_stats() : expiry(0) {}
    };

    enum hint
    {
      HINT_NONE    = 0x0,
      HINT_IS_DIR  = 0x1,
      HINT_IS_FILE = 0x2
    };

    typedef std::map<std::string, file_stats> stats_map;

    int get_stats(const std::string &path, struct stat *s, int hints);
    bool get_cached_stats(const std::string &path, struct stat *s);
    void update_stats_cache(const std::string &path, const struct stat *s);

    void prefill_stats(const std::string &path, int hints);

    std::string _bucket;
    stats_map _stats_map;  
    boost::mutex _stats_mutex;
    pugi::xpath_query _prefix_query;
    pugi::xpath_query _key_query;
    async_queue _async_queue;
  };
}

#endif
