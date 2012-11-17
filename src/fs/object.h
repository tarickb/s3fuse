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

#ifndef S3_FS_OBJECT_H
#define S3_FS_OBJECT_H

#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <string>
#include <vector>
#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>

#include "base/static_list.h"
#include "fs/xattr.h"
#include "threads/pool.h"

namespace s3
{
  namespace base
  {
    class request;
  }

  namespace fs
  {
    class object : public boost::enable_shared_from_this<object>
    {
    public:
      typedef boost::shared_ptr<object> ptr;

      typedef object * (*type_checker_fn)(const std::string &path, const boost::shared_ptr<base::request> &req);
      typedef base::static_list<type_checker_fn> type_checker_list;

      static std::string build_url(const std::string &path);
      static ptr create(const std::string &path, const boost::shared_ptr<base::request> &req);
      static int remove_by_url(const boost::shared_ptr<base::request> &req, const std::string &url);
      static int copy_by_path(const boost::shared_ptr<base::request> &req, const std::string &from, const std::string &to);

      virtual ~object();

      inline bool is_intact() const { return (_etag == _last_update_etag); }
      inline bool is_expired() const { return (_expiry == 0 || time(NULL) >= _expiry); }

      virtual bool is_removable();

      inline const std::string & get_path() const { return _path; }
      inline const std::string & get_content_type() const { return _content_type; }
      inline const std::string & get_url() const { return _url; }
      inline const std::string & get_etag() const { return _etag; }
      inline mode_t get_mode() const { return _stat.st_mode; }
      inline mode_t get_type() const { return _stat.st_mode & S_IFMT; }

      void get_metadata_keys(std::vector<std::string> *keys);
      int get_metadata(const std::string &key, char *buffer, size_t max_size);
      int set_metadata(const std::string &key, const char *value, size_t size, int flags = 0);
      int remove_metadata(const std::string &key);

      inline void set_uid(uid_t uid) { _stat.st_uid = uid; }
      inline void set_gid(gid_t gid) { _stat.st_gid = gid; }
      inline void set_mtime(time_t mtime) { _stat.st_mtime = mtime; }

      void set_mode(mode_t mode);

      virtual void copy_stat(struct stat *s);

      int commit(const boost::shared_ptr<base::request> &req);

      virtual int remove(const boost::shared_ptr<base::request> &req);
      virtual int rename(const boost::shared_ptr<base::request> &req, const std::string &to);

      inline int commit()
      {
        return threads::pool::call(
          threads::PR_REQ_0, 
          boost::bind(&object::commit, shared_from_this(), _1));
      }

      inline int remove()
      {
        return threads::pool::call(
          threads::PR_REQ_0, 
          boost::bind(&object::remove, shared_from_this(), _1));
      }

      inline int rename(const std::string &to)
      {
        return threads::pool::call(
          threads::PR_REQ_0, 
          boost::bind(&object::rename, shared_from_this(), _1, to));
      }

    protected:
      object(const std::string &path);

      virtual void init(const boost::shared_ptr<base::request> &req);

      virtual void set_request_headers(const boost::shared_ptr<base::request> &req);
      virtual void set_request_body(const boost::shared_ptr<base::request> &req);

      inline void set_url(const std::string &url) { _url = url; }
      inline void set_content_type(const std::string &content_type) { _content_type = content_type; }
      inline void set_object_type(mode_t mode) { _stat.st_mode |= mode & S_IFMT; }
      inline void set_etag(const std::string &etag) { _etag = etag; }

      inline xattr_map * get_metadata() { return &_metadata; }
      inline const struct stat * get_stat() const { return &_stat; }

      inline void expire() { _expiry = 0; }

    private:
      boost::mutex _mutex;

      // should only be modified during init()
      std::string _path;
      std::string _last_update_etag;
      std::string _content_type;
      std::string _url;

      // unprotected
      std::string _etag;
      struct stat _stat;
      time_t _expiry;

      // protected by _mutex
      xattr_map _metadata;
    };
  }
}

#endif
