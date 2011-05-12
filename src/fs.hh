#ifndef S3_FS_HH
#define S3_FS_HH

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <time.h>

#include <string>
#include <boost/bind.hpp>

#include "object_cache.hh"
#include "open_file_map.hh"
#include "thread_pool.hh"

namespace s3
{
  class fs
  {
  public:
    fs();
    ~fs();

    inline int get_stats(const std::string &path, struct stat *s)
    {
      return _tp_fg->call(boost::bind(&fs::__get_stats, this, _1, path, s, HINT_NONE));
    }

    inline int read_directory(const std::string &path, fuse_fill_dir_t filler, void *buf)
    {
      return _tp_fg->call(boost::bind(&fs::__read_directory, this, _1, path, filler, buf));
    }

    inline int create_file(const std::string &path, mode_t mode)
    {
      return _tp_fg->call(boost::bind(&fs::__create_object, this, _1, path, OT_FILE, mode, ""));
    }

    inline int create_directory(const std::string &path, mode_t mode)
    {
      return _tp_fg->call(boost::bind(&fs::__create_object, this, _1, path, OT_DIRECTORY, mode, ""));
    }

    inline int create_symlink(const std::string &path, const std::string &target)
    {
      return _tp_fg->call(boost::bind(&fs::__create_object, this, _1, path, OT_SYMLINK, 0, target));
    }

    inline int change_owner(const std::string &path, uid_t uid, gid_t gid)
    {
      return change_metadata(path, -1, uid, gid, -1);
    }

    inline int change_mode(const std::string &path, mode_t mode)
    {
      return change_metadata(path, mode, -1, -1, -1);
    }

    inline int change_mtime(const std::string &path, time_t mtime)
    {
      return change_metadata(path, -1, -1, -1, mtime);
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

    inline int read_symlink(const std::string &path, std::string *target)
    {
      return _tp_fg->call(boost::bind(&fs::__read_symlink, this, _1, path, target));
    }

    inline int open(const std::string &path, uint64_t *handle) { return _open_files.open(_object_cache.get(path), handle); }
    inline int release(uint64_t handle) { return _open_files.release(handle); }
    inline int flush(uint64_t handle) { return _open_files.flush(handle); }
    inline int read(uint64_t handle, char *buffer, size_t size, off_t offset) { return _open_files.read(handle, buffer, size, offset); }
    inline int write(uint64_t handle, const char *buffer, size_t size, off_t offset) { return _open_files.write(handle, buffer, size, offset); }

  private:
    inline int change_metadata(const std::string &path, mode_t mode, uid_t uid, gid_t gid, time_t mtime)
    {
      return _tp_fg->call(boost::bind(&fs::__change_metadata, this, _1, path, mode, uid, gid, mtime));
    }

    int rename_children(const std::string &from, const std::string &to);

    int  copy_file         (const request_ptr &req, const std::string &from, const std::string &to);
    int  remove_object     (const request_ptr &req, const std::string &url);
    bool is_directory_empty(const request_ptr &req, const std::string &path);

    int  __change_metadata (const request_ptr &req, const std::string &path, mode_t mode, uid_t uid, gid_t gid, time_t mtime);
    int  __create_object   (const request_ptr &req, const std::string &path, object_type type, mode_t mode, const std::string &symlink_target);
    int  __get_stats       (const request_ptr &req, const std::string &path, struct stat *s, int hints);
    int  __prefill_stats   (const request_ptr &req, const std::string &path, int hints);
    int  __read_directory  (const request_ptr &req, const std::string &path, fuse_fill_dir_t filler, void *buf);
    int  __read_symlink    (const request_ptr &req, const std::string &path, std::string *target);
    int  __remove_object   (const request_ptr &req, const std::string &path);
    int  __rename_object   (const request_ptr &req, const std::string &from, const std::string &to);

    thread_pool::ptr _tp_fg, _tp_bg;
    object_cache _object_cache;
    open_file_map _open_files;
  };
}

#endif
