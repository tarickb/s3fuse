/*
 * fs/glacier.h
 * -------------------------------------------------------------------------
 * Glacier auto-archive support class.
 * -------------------------------------------------------------------------
 *
 * Copyright (c) 2012, Tarick Bedeir.
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

#ifndef S3_FS_GLACIER_H
#define S3_FS_GLACIER_H

#include <string>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>

#include "threads/pool.h"

namespace s3
{
  namespace fs
  {
    class object;
    class xattr;

    class glacier
    {
    public:
      typedef boost::shared_ptr<glacier> ptr;

      inline static ptr create(const object *obj, const boost::shared_ptr<base::request> &req)
      {
        ptr p(new glacier(obj));

        p->read_restore_status(req);

        return p;
      }

      inline const boost::shared_ptr<xattr> & get_storage_class_xattr() { return _storage_class_xattr; }
      inline const boost::shared_ptr<xattr> & get_restore_ongoing_xattr() { return _restore_ongoing_xattr; }
      inline const boost::shared_ptr<xattr> & get_restore_expiry_xattr() { return _restore_expiry_xattr; }
      inline const boost::shared_ptr<xattr> & get_request_restore_xattr() { return _request_restore_xattr; }

    private:
      glacier(const object *obj);

      inline int get_storage_class_value(std::string *out)
      {
        if (_storage_class.empty()) {
          int r = threads::pool::call(
            threads::PR_REQ_1,
            boost::bind(&glacier::query_storage_class, this, _1));

          if (r)
            return r;
        }

        *out = _storage_class;

        return 0;
      }

      inline int get_restore_ongoing_value(std::string *out)
      {
        *out = _restore_ongoing;

        return 0;
      }

      inline int get_restore_expiry_value(std::string *out)
      {
        *out = _restore_expiry;

        return 0;
      }

      inline int set_request_restore_value(const std::string &days_str)
      {
        int days;

        try {
          days = boost::lexical_cast<int>(days_str);
        } catch (...) {
          return -EINVAL;
        }

        return threads::pool::call(
          threads::PR_REQ_1,
          boost::bind(&glacier::start_restore, this, _1, days));
      }

      void read_restore_status(const boost::shared_ptr<base::request> &req);
      int query_storage_class(const boost::shared_ptr<base::request> &req);
      int start_restore(const boost::shared_ptr<base::request> &req, int days);

      const object *_object;
      std::string _storage_class, _restore_ongoing, _restore_expiry;
      boost::shared_ptr<xattr> _storage_class_xattr, _restore_ongoing_xattr, _restore_expiry_xattr, _request_restore_xattr;
    };
  }
}

#endif
