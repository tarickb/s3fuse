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

#include <string>
#include <vector>
#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>

#include "xattr.h"

namespace s3
{
  class request;

  class object
  {
  public:
    typedef boost::shared_ptr<object> ptr;

    class type_checker
    {
    public:
      typedef object * (*fn)(const std::string &path, const boost::shared_ptr<request> &req);

      type_checker(fn checker, int priority);
    };

    static std::string build_url(const std::string &path);
    static ptr create(const std::string &path, const boost::shared_ptr<request> &req);

    virtual ~object();

    virtual bool is_valid();

    inline const std::string & get_path() const { return _path; }
    inline const std::string & get_content_type() const { return _content_type; }
    inline const std::string & get_url() const { return _url; }
    inline mode_t get_mode() const { return _stat.st_mode; }

    inline std::string get_etag()
    {
      boost::mutex::scoped_lock lock(_mutex);
      
      return _etag;
    }

    void get_metadata_keys(std::vector<std::string> *keys);
    int get_metadata(const std::string &key, char *buffer, size_t max_size);

    inline void set_uid(uid_t uid) { _stat.st_uid = uid; }
    inline void set_gid(gid_t gid) { _stat.st_gid = gid; }
    inline void set_mtime(time_t mtime) { _stat.st_mtime = mtime; }

    virtual void copy_stat(struct stat *s);

    void set_mode(mode_t mode);

    virtual void set_request_headers(const boost::shared_ptr<request> &req);

    int set_metadata(const std::string &key, const char *value, size_t size, int flags = 0);
    int remove_metadata(const std::string &key);
    int commit_metadata(const boost::shared_ptr<request> &req);

  protected:
    object(const std::string &path);

    virtual void init(const boost::shared_ptr<request> &req);

    inline void set_etag(const std::string &etag)
    {
      boost::mutex::scoped_lock lock(_mutex);

      _etag = etag;
    }

    inline void set_url(const std::string &url) { _url = url; }
    inline void set_content_type(const std::string &content_type) { _content_type = content_type; }
    inline void set_object_type(mode_t mode) { _stat.st_mode |= mode; }

    inline xattr_map * get_metadata() { return &_metadata; }
    inline const struct stat * get_stat() const { return &_stat; }

    inline void expire() { _expiry = 0; }

  private:
    boost::mutex _mutex;

    // should only be modified during init()
    std::string _path;
    std::string _mtime_etag;
    std::string _content_type;
    std::string _url;

    // unprotected
    struct stat _stat;
    time_t _expiry;

    // protected by _mutex
    std::string _etag;
    xattr_map _metadata;
  };
}

#endif
