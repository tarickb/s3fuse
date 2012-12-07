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

        p->set_restore_status(req);

        return p;
      }

      inline const boost::shared_ptr<xattr> & get_storage_class() { return _storage_class_xattr; }
      inline const boost::shared_ptr<xattr> & get_restore_ongoing() { return _ongoing_xattr; }
      inline const boost::shared_ptr<xattr> & get_restore_expiry() { return _expiry_xattr; }

    private:
      glacier(const object *obj);

      inline int get_storage_class_value(std::string *out)
      {
        if (_storage_class.empty()) {
          int r = threads::pool::call(
            threads::PR_REQ_1,
            boost::bind(&glacier::read_storage_class, this, _1));

          if (r)
            return r;
        }

        *out = _storage_class;

        return 0;
      }

      inline int get_ongoing_value(std::string *out)
      {
        *out = _ongoing;

        return 0;
      }

      inline int get_expiry_value(std::string *out)
      {
        *out = _expiry;

        return 0;
      }

      void set_restore_status(const boost::shared_ptr<base::request> &req);
      int read_storage_class(const boost::shared_ptr<base::request> &req);

      const object *_object;
      std::string _storage_class, _ongoing, _expiry;
      boost::shared_ptr<xattr> _storage_class_xattr, _ongoing_xattr, _expiry_xattr;
    };
  }
}

#endif
