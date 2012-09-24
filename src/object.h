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
  class xattr;

  class object : public boost::enable_shared_from_this<object>
  {
  public:
    typedef boost::shared_ptr<object> ptr;

    static std::string build_url(const std::string &path);
    static ptr create(const std::string &path, const boost::shared_ptr<request> &req);

    virtual ~object();

    inline const std::string & get_path() { return _path; }
    inline const std::string & get_content_type() { return _content_type; }
    inline const std::string & get_url() { return _url; }
    inline mode_t get_mode() { return _stat.st_mode; }

    void get_metadata_keys(std::vector<std::string> *keys);
    int get_metadata(const std::string &key, char *buffer, size_t max_size);

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

    inline void copy_stat(struct stat *s)
    {
      memcpy(s, &_stat, sizeof(_stat));

      adjust_stat(s);
    }

    void set_mode(mode_t mode);
    int set_metadata(const std::string &key, const char *value, size_t size, int flags = 0);
    int remove_metadata(const std::string &key);

    int commit_metadata(const boost::shared_ptr<request> &req);

  protected:
    object(const std::string &path);

    virtual void init(const boost::shared_ptr<request> &req);
    virtual void adjust_stat(struct stat *s);

    std::string _content_type;
    std::string _url;

    struct stat _stat;

  private:
    friend class request; // for request_*
    friend class object_cache; // for is_valid, set_open_file, get_open_file

    typedef boost::shared_ptr<xattr> xattr_ptr;
    typedef std::map<std::string, xattr_ptr> xattr_map;

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

    boost::mutex _metadata_mutex, _open_file_mutex;

    std::string _path, _mtime_etag;
    time_t _expiry;

    // protected by _metadata_mutex
    xattr_map _metadata;
    std::string _etag, _md5, _md5_etag;

    // protected by _open_file_mutex
    open_file::ptr _open_file;
  };
}

#endif
