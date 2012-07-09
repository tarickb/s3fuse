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
#include <boost/function.hpp>
#include <boost/smart_ptr.hpp>

#include "open_file.h"
#include "util.h"

namespace s3
{
  class request;

  enum object_type
  {
    OT_INVALID,
    OT_FILE,
    OT_DIRECTORY,
    OT_SYMLINK
  };

  class object : public boost::enable_shared_from_this<object>
  {
  public:
    typedef boost::shared_ptr<object> ptr;

    typedef std::list<std::string> dir_cache;
    typedef boost::shared_ptr<dir_cache> dir_cache_ptr;

    typedef boost::function1<void, const std::string &> dir_filler_function;

    static std::string get_bucket_url();
    static std::string build_url(const std::string &path, object_type type);

    object(const std::string &path, object_type type = OT_INVALID);

    int set_metadata(const std::string &key, const char *value, size_t size, int flags = 0);
    void get_metadata_keys(std::vector<std::string> *keys);
    int get_metadata(const std::string &key, char *buffer, size_t max_size);
    int remove_metadata(const std::string &key);

    void set_mode(mode_t mode);

    inline void set_uid(uid_t uid) { _stat.st_uid = uid; }
    inline void set_gid(gid_t gid) { _stat.st_gid = gid; }
    inline void set_mtime(time_t mtime) { _stat.st_mtime = mtime; }

    inline void set_md5(const std::string &md5, const std::string &etag)
    {
      boost::mutex::scoped_lock lock(_metadata_mutex);

      _md5 = md5;
      _md5_etag = etag;
      _etag = etag;
    }

    inline object_type get_type() { return _type; }
    inline const std::string & get_path() { return _path; }
    inline const std::string & get_content_type() { return _content_type; }

    inline bool is_directory_cached()
    {
      boost::mutex::scoped_lock lock(_metadata_mutex);

      return _dir_cache; 
    }

    inline void set_directory_cache(const dir_cache_ptr &dc)
    {
      boost::mutex::scoped_lock lock(_metadata_mutex);

      if (_type != OT_DIRECTORY)
        throw std::runtime_error("cannot set directory cache on an object that isn't a directory!");

      _dir_cache = dc;
    }

    inline void fill_directory(const dir_filler_function &filler)
    {
      boost::mutex::scoped_lock lock(_metadata_mutex);
      dir_cache_ptr dc;

      // make local copy of _dir_cache so we don't hold the metadata mutex
      // longer than necessary.

      dc = _dir_cache;
      lock.unlock();

      if (!dc)
        throw std::runtime_error("directory cache not set. cannot fill directory.");

      for (dir_cache::const_iterator itor = dc->begin(); itor != dc->end(); ++itor)
        filler(*itor);
    }

    inline std::string get_etag()
    {
      boost::mutex::scoped_lock lock(_metadata_mutex);

      return _etag; 
    }

    inline std::string get_md5()
    {
      boost::mutex::scoped_lock lock(_metadata_mutex);

      return _md5; 
    }

    inline const std::string & get_url() { return _url; }

    inline size_t get_size()
    {
      boost::mutex::scoped_lock lock(_open_file_mutex);

      if (_open_file) {
        struct stat s;

        if (fstat(_open_file->get_fd(), &s) == 0)
          _stat.st_size = s.st_size;
      }

      return _stat.st_size;
    }

    inline void copy_stat(struct stat *s)
    {
      memcpy(s, &_stat, sizeof(_stat));
      s->st_size = get_size();
    }

    int commit_metadata(const boost::shared_ptr<request> &req);

  private:
    friend class request; // for request_*
    friend class object_cache; // for is_valid, set_open_file, get_open_file

    class meta_value;

    typedef boost::shared_ptr<meta_value> meta_value_ptr;
    typedef std::map<std::string, meta_value_ptr> meta_map;

    inline bool is_valid() { return (_expiry > 0 && time(NULL) < _expiry); }

    inline const open_file::ptr & get_open_file()
    {
      boost::mutex::scoped_lock lock(_open_file_mutex);

      return _open_file;
    }

    inline void set_open_file(const open_file::ptr &open_file)
    {
      boost::mutex::scoped_lock lock(_open_file_mutex);

      _open_file = open_file;
    }

    void init_stat();

    void request_init();
    void request_process_header(const std::string &key, const std::string &value);
    void request_process_response(request *req);
    void request_set_meta_headers(request *req);

    boost::mutex _metadata_mutex, _open_file_mutex;

    object_type _type;
    std::string _path, _url, _content_type, _mtime_etag;
    time_t _expiry;
    struct stat _stat;

    // protected by _metadata_mutex
    meta_map _metadata;
    std::string _etag, _md5, _md5_etag;
    dir_cache_ptr _dir_cache;

    // protected by _open_file_mutex
    open_file::ptr _open_file;
  };
}

#endif
