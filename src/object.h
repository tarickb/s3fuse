/*
 * object.h
 * -------------------------------------------------------------------------
 * Represents an S3 object (a file, a directory, or a symlink).
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

#ifndef S3_OBJECT_H
#define S3_OBJECT_H

#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <map>
#include <string>
#include <vector>
#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>

namespace s3
{
  class locked_object;
  class object_builder;
  class object_cache;
  class request;

  enum object_type
  {
    OT_INVALID,
    OT_FILE,
    OT_DIRECTORY,
    OT_SYMLINK
  };

  typedef uint64_t object_handle;

  class object : public boost::noncopyable
  {
  public:
    #define LOCK boost::mutex::scoped_lock lock(_mutex)

    typedef boost::shared_ptr<object> ptr;

    static std::string get_bucket_url();
    static std::string build_url(const std::string &path, object_type type);
    static ptr create(const std::string &path, object_type type);

    inline void set_uid(uid_t uid) { _stat.st_uid = uid; }
    inline void set_gid(gid_t gid) { _stat.st_gid = gid; }
    inline void set_mtime(time_t mtime) { _stat.st_mtime = mtime; }

    inline const std::string & get_url() { return _url; }
    inline const std::string & get_path() { return _path; }

    inline std::string get_content_type() { LOCK; return _content_type; }
    inline void set_content_type(const std::string &content_type) { LOCK; _content_type = content_type; }

    inline const std::string & get_etag() { return _etag; }

    inline bool is_valid() { return _expiry > 0 && time(NULL) < _expiry; }

    inline bool is_removable() { LOCK; return _lock_count == 0 && _ref_count == 0; }
    inline bool is_locked() { LOCK; return _lock_count > 0; }
    inline bool is_in_use() { LOCK; return _ref_count > 0; }

    void set_mode(mode_t mode);

    int commit_metadata(const boost::shared_ptr<request> &req);

    virtual object_type get_type() = 0;
    virtual mode_t get_mode() = 0;

    virtual void copy_stat(struct stat *s);

    virtual int set_metadata(const std::string &key, const std::string &value, int flags = 0);
    virtual void get_metadata_keys(std::vector<std::string> *keys);
    virtual int get_metadata(const std::string &key, std::string *value);
    virtual int remove_metadata(const std::string &key);

    virtual void set_meta_headers(const boost::shared_ptr<request> &req);

  protected:
    object(const std::string &path);

    void init();

    inline boost::mutex & get_mutex() { return _mutex; }
    inline void invalidate() { _expiry = 0; }

    virtual void build_process_header(const boost::shared_ptr<request> &req, const std::string &key, const std::string &value);
    virtual void build_finalize(const boost::shared_ptr<request> &req);

  private:
    friend class locked_object; // for lock, unlock
    friend class file_handle_map; // for set_handle, add_ref, release
    friend class object_builder; // for build_*

    typedef std::map<std::string, std::string> meta_map;

    inline void lock() { LOCK; ++_lock_count; }
    inline void unlock() { LOCK; --_lock_count; }

    inline void set_handle(object_handle handle) { LOCK; _handle = handle; }

    inline void add_ref(object_handle *handle)
    {
      LOCK;

      ++_ref_count;

      if (handle)
        *handle = _handle;
    }

    inline void release() { LOCK; --_ref_count; }

    boost::mutex _mutex;
    std::string _path, _url, _etag, _mtime_etag;
    time_t _expiry;
    struct stat _stat;
    object_handle _handle;
    uint64_t _lock_count, _ref_count;

    // protected by _mutex
    meta_map _metadata;
    std::string _content_type;

    #undef LOCK
  };
}


#endif
