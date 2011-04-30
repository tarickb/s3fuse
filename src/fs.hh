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

    inline int get_stats(const std::string &path, struct stat *s)
    {
      return _tp_fg->call(boost::bind(&fs::__get_stats, this, _1, path, s, HINT_NONE));
    }

    inline int read_directory(const std::string &path, fuse_fill_dir_t filler, void *buf)
    {
      return _tp_fg->call(boost::bind(&fs::__read_directory, this, _1, path, filler, buf));
    }

    inline int create_object(const std::string &path, mode_t mode)
    {
      return _tp_fg->call(boost::bind(&fs::__create_object, this, _1, path, mode));
    }

    inline int change_metadata(const std::string &path, mode_t mode, uid_t uid, gid_t gid)
    {
      return _tp_fg->call(boost::bind(&fs::__change_metadata, this, _1, path, mode, uid, gid));
    }

    inline int remove_file(const std::string &path)
    {
      return _tp_fg->call(boost::bind(&fs::__remove_object, this, _1, path));
    }

    inline int remove_directory(const std::string &path)
    {
      return _tp_fg->call(boost::bind(&fs::__remove_object, this, _1, path));
    }

    inline int rename_object(const std::string &from, const std::string &to)
    {
      return _tp_fg->call(boost::bind(&fs::__rename_object, this, _1, from, to));
    }

    inline int open(const std::string &path, uint64_t *handle) { return _open_file_cache.open(_object_cache.get(path), handle); }
    inline int close(uint64_t handle) { return _open_file_cache.close(handle); }
    inline int flush(uint64_t handle) { return _open_file_cache.flush(handle); }
    inline int read(uint64_t handle, char *buffer, size_t size, off_t offset) { return _open_file_cache.read(handle, buffer, size, offset); }
    inline int write(uint64_t handle, const char *buffer, size_t size, off_t offset) { return _open_file_cache.write(handle, buffer, size, offset); }

  private:
    int __get_stats        (const request_ptr &req, const std::string &path, struct stat *s, int hints);
    int __prefill_stats    (const request_ptr &req, const std::string &path, int hints);
    int __read_directory   (const request_ptr &req, const std::string &path, fuse_fill_dir_t filler, void *buf);
    int __create_object    (const request_ptr &req, const std::string &path, mode_t mode);
    int __change_metadata  (const request_ptr &req, const std::string &path, mode_t mode, uid_t uid, gid_t gid);
    int __remove_object    (const request_ptr &req, const std::string &path);
    int __rename_object    (const request_ptr &req, const std::string &from, const std::string &to);

    int remove_object(const request::ptr &req, const object::ptr &obj);

    pugi::xpath_query _prefix_query;
    pugi::xpath_query _key_query;
    thread_pool::ptr _tp_fg, _tp_bg;
    object_cache _object_cache;
    open_file_cache _open_file_cache;
  };
}

#endif
