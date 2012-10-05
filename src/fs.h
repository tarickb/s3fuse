/*
 * fs.h
 * -------------------------------------------------------------------------
 * Core S3 file system.
 * -------------------------------------------------------------------------
 *
 * Copyright (c) 2011, Tarick Bedeir.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef S3_FS_H
#define S3_FS_H

#include <time.h>

#include <string>
#include <boost/bind.hpp>

#include "file.h"
#include "object_cache.h"
#include "thread_pool.h"

namespace s3
{
  class fs
  {
  public:
    fs();
    ~fs();

    inline int get_stats(const std::string &path, struct stat *s)
    {
      return thread_pool::call(thread_pool::PR_FG, boost::bind(&fs::__get_stats, this, _1, path, s, HINT_NONE));
    }

    inline int read_directory(const std::string &path, const object::dir_filler_function &filler)
    {
      return thread_pool::call(thread_pool::PR_FG, boost::bind(&fs::__read_directory, this, _1, path, filler));
    }

    inline int create_file(const std::string &path, mode_t mode, uid_t uid, gid_t gid)
    {
      return thread_pool::call(thread_pool::PR_FG, boost::bind(&fs::__create_object, this, _1, path, OT_FILE, mode, uid, gid, ""));
    }

    inline int create_directory(const std::string &path, mode_t mode, uid_t uid, gid_t gid)
    {
      return thread_pool::call(thread_pool::PR_FG, boost::bind(&fs::__create_object, this, _1, path, OT_DIRECTORY, mode, uid, gid, ""));
    }

    inline int create_symlink(const std::string &path, uid_t uid, gid_t gid, const std::string &target)
    {
      return thread_pool::call(thread_pool::PR_FG, boost::bind(&fs::__create_object, this, _1, path, OT_SYMLINK, 0, uid, gid, target));
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
      return thread_pool::call(thread_pool::PR_FG, boost::bind(&fs::__remove_object, this, _1, path));
    }

    inline int remove_directory(const std::string &path)
    {
      return thread_pool::call(thread_pool::PR_FG, boost::bind(&fs::__remove_object, this, _1, path));
    }

    inline int rename_object(const std::string &from, const std::string &to)
    {
      return thread_pool::call(thread_pool::PR_FG, boost::bind(&fs::__rename_object, this, _1, from, to));
    }

    inline int read_symlink(const std::string &path, std::string *target)
    {
      return thread_pool::call(thread_pool::PR_FG, boost::bind(&fs::__read_symlink, this, _1, path, target));
    }

    int set_attr(const std::string &path, const std::string &name, const char *value, size_t size, int flags)
    {
      return thread_pool::call(thread_pool::PR_FG, boost::bind(&fs::__set_attr, this, _1, path, name, value, size, flags));
    }

    int remove_attr(const std::string &path, const std::string &name)
    {
      return thread_pool::call(thread_pool::PR_FG, boost::bind(&fs::__remove_attr, this, _1, path, name));
    }

    inline int get_attr(const std::string &path, const std::string &name, char *value, size_t max_size)
    {
      object::ptr obj = _object_cache->get(path);

      if (!obj)
        return -ENOENT;

      return obj->get_metadata(name, value, max_size);
    }

    inline int list_attr(const std::string &path, std::vector<std::string> *attrs)
    {
      object::ptr obj = _object_cache->get(path);

      if (!obj)
        return -ENOENT;

      obj->get_metadata_keys(attrs);

      return 0;
    }

    inline int open(const std::string &path, uint64_t *handle)
    {
      return file::open(path, _object_cache, handle);
    }

    // TODO: remove
    /*
    inline int truncate_by_path(const std::string &path, off_t offset) 
    {
      uint64_t handle;
      int r;

      r = open(path, &handle);

      if (r)
        return r;

      r = truncate(handle, offset);
      release(handle);

      return r;
    }
    */

  private:
    inline int change_metadata(const std::string &path, mode_t mode, uid_t uid, gid_t gid, time_t mtime)
    {
      return thread_pool::call(thread_pool::PR_FG, boost::bind(&fs::__change_metadata, this, _1, path, mode, uid, gid, mtime));
    }

    void invalidate_parent(const std::string &path);

    int  copy_file         (const request_ptr &req, const std::string &from, const std::string &to);
    int  remove_object     (const request_ptr &req, const std::string &url);
    int  rename_children   (const request_ptr &req, const std::string &from, const std::string &to);
    bool is_directory_empty(const request_ptr &req, const std::string &path);

    int  __change_metadata (const request_ptr &req, const std::string &path, mode_t mode, uid_t uid, gid_t gid, time_t mtime);
    int  __create_object   (const request_ptr &req, const std::string &path, object_type type, mode_t mode, uid_t uid, gid_t gid, const std::string &symlink_target);
    int  __get_stats       (const request_ptr &req, const std::string &path, struct stat *s, int hints);
    int  __prefill_stats   (const request_ptr &req, const std::string &path, int hints);
    int  __read_directory  (const request_ptr &req, const std::string &path, const object::dir_filler_function &filler);
    int  __read_symlink    (const request_ptr &req, const std::string &path, std::string *target);
    int  __remove_attr     (const request_ptr &req, const std::string &path, const std::string &name);
    int  __remove_object   (const request_ptr &req, const std::string &path);
    int  __rename_object   (const request_ptr &req, const std::string &from, const std::string &to);
    int  __set_attr        (const request_ptr &req, const std::string &path, const std::string &name, const char *value, size_t size, int flags);

    object_cache::ptr _object_cache;
  };
}

#endif
