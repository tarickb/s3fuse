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

#include "object_cache.hh"
#include "open_file_cache.hh"
#include "request.hh"
#include "thread_pool.hh"
#include "work_item.hh"

namespace s3
{
  class fs
  {
  public:
    fs();

    inline int get_stats(const std::string &path, struct stat *s) { ASYNC_CALL(_tp_fg, fs, get_stats, path, s, HINT_NONE); }
    inline int read_directory(const std::string &path, fuse_fill_dir_t filler, void *buf) { ASYNC_CALL(_tp_fg, fs, read_directory, path, filler, buf); }
    inline int create_object(const std::string &path, mode_t mode) { ASYNC_CALL(_tp_fg, fs, create_object, path, mode); }
    inline int change_metadata(const std::string &path, mode_t mode, uid_t uid, gid_t gid) { ASYNC_CALL(_tp_fg, fs, change_metadata, path, mode, uid, gid); }
    inline int remove_file(const std::string &path) { ASYNC_CALL(_tp_fg, fs, remove_object, path); }
    inline int remove_directory(const std::string &path) { ASYNC_CALL(_tp_fg, fs, remove_object, path); }
    inline int rename_object(const std::string &from, const std::string &to) { ASYNC_CALL(_tp_fg, fs, rename_object, from, to); }

    inline int open(const std::string &path, uint64_t *handle)
    {
      object::ptr obj = _object_cache.get(path);

      return obj ? _open_file_cache.open(obj, handle) : -ENOENT;
    }

    inline int close(uint64_t handle) { return _open_file_cache.close(handle); }
    inline int flush(uint64_t handle) { return _open_file_cache.flush(handle); }
    inline int read(uint64_t handle, char *buffer, size_t size, off_t offset) { return _open_file_cache.read(handle, buffer, size, offset); }
    inline int write(uint64_t handle, const char *buffer, size_t size, off_t offset) { return _open_file_cache.write(handle, buffer, size, offset); }

  private:
    ASYNC_DECL(get_stats,       const std::string &path, struct stat *s, int hints);
    ASYNC_DECL(prefill_stats,   const std::string &path, int hints);
    ASYNC_DECL(read_directory,  const std::string &path, fuse_fill_dir_t filler, void *buf);
    ASYNC_DECL(create_object,   const std::string &path, mode_t mode);
    ASYNC_DECL(change_metadata, const std::string &path, mode_t mode, uid_t uid, gid_t gid);
    ASYNC_DECL(remove_object,   const std::string &path);
    ASYNC_DECL(rename_object,   const std::string &from, const std::string &to);

    int remove_object(const request::ptr &req, const object::ptr &obj);

    pugi::xpath_query _prefix_query;
    pugi::xpath_query _key_query;
    thread_pool::ptr _tp_fg, _tp_bg;
    object_cache _object_cache;
    open_file_cache _open_file_cache;
  };
}

#endif
