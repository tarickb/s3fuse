#ifndef S3_FS_HH
#define S3_FS_HH

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <time.h>

#include <map>
#include <string>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <pugixml/pugixml.hpp>

#include "request.hh"
#include "stats_cache.hh"
#include "thread_pool.hh"
#include "work_item.hh"

#define ASYNC_CALL(fn, ...) \
  do { \
    work_item::ptr wi(new work_item(boost::bind(&fs::__ ## fn, this, _1, ## __VA_ARGS__))); \
    \
    _thread_pool.post(wi); \
    return wi->wait(); \
  } while (0)

#define ASYNC_CALL_NONBLOCK(fn, ...) \
  do { \
    work_item::ptr wi(new work_item(boost::bind(&fs::__ ## fn, this, _1, ## __VA_ARGS__))); \
    \
    _thread_pool.post(wi); \
  } while (0)

#define ASYNC_DECL(fn, ...) int __ ## fn (const request::ptr &req, ## __VA_ARGS__);
#define ASYNC_DEF(fn, ...) int fs::__ ## fn (const request::ptr &req, ## __VA_ARGS__)

namespace s3
{
  class fs
  {
  public:
    fs(const std::string &bucket);
    ~fs();

    // the cast to std::string * is required to prevent boost from trying to pass a long int
    inline int get_stats(const std::string &path, struct stat *s) { ASYNC_CALL(get_stats, path, static_cast<std::string *>(NULL), s, HINT_NONE); }
    inline int read_directory(const std::string &path, fuse_fill_dir_t filler, void *buf) { ASYNC_CALL(read_directory, path, filler, buf); }
    inline int create_object(const std::string &path, mode_t mode) { ASYNC_CALL(create_object, path, mode); }
    inline int change_metadata(const std::string &path, mode_t mode, uid_t uid, gid_t gid) { ASYNC_CALL(change_metadata, path, mode, uid, gid); }

  private:
    enum hint
    {
      HINT_NONE    = 0x0,
      HINT_IS_DIR  = 0x1,
      HINT_IS_FILE = 0x2
    };

    ASYNC_DECL(get_stats,       const std::string &path, std::string *etag, struct stat *s, int hints);
    ASYNC_DECL(prefill_stats,   const std::string &path, int hints);
    ASYNC_DECL(read_directory,  const std::string &path, fuse_fill_dir_t filler, void *buf);
    ASYNC_DECL(create_object,   const std::string &path, mode_t mode);
    ASYNC_DECL(change_metadata, const std::string &path, mode_t mode, uid_t uid, gid_t gid);

    std::string _bucket;
    pugi::xpath_query _prefix_query;
    pugi::xpath_query _key_query;
    thread_pool _thread_pool;
    stats_cache _stats_cache;
  };
}

#endif
