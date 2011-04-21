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
#include "s3_stats_cache.hh"

namespace s3
{
  class fs
  {
  public:
    fs(const std::string &bucket);
    ~fs();

    inline int get_stats(const std::string &path, struct stat *s)
    {
      return get_stats(path, NULL, s, HINT_NONE);
    }

    int read_directory(const std::string &path, fuse_fill_dir_t filler, void *buf);
    int create_object(const std::string &path, mode_t mode);
    int change_metadata(const std::string &path, mode_t mode, uid_t uid, gid_t gid);

  private:
    enum hint
    {
      HINT_NONE    = 0x0,
      HINT_IS_DIR  = 0x1,
      HINT_IS_FILE = 0x2
    };

    int get_stats(const std::string &path, std::string *etag, struct stat *s, int hints);

    void async_prefill_stats(const std::string &path, int hints);

    std::string _bucket;
    pugi::xpath_query _prefix_query;
    pugi::xpath_query _key_query;
    async_queue _async_queue;
    stats_cache _stats_cache;
  };
}

#endif
