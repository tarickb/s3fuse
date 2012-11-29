/*
 * fs/symlink.h
 * -------------------------------------------------------------------------
 * Represents a symbolic link object.
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

#ifndef S3_FS_SYMLINK_H
#define S3_FS_SYMLINK_H

#include "fs/object.h"
#include "threads/pool.h"

namespace s3
{
  namespace fs
  {
    class symlink : public object
    {
    public:
      typedef boost::shared_ptr<symlink> ptr;

      symlink(const std::string &path);
      virtual ~symlink();

      inline ptr shared_from_this()
      {
        return boost::static_pointer_cast<symlink>(object::shared_from_this());
      }

      inline int read(std::string *target)
      {
        boost::mutex::scoped_lock lock(_mutex);

        if (_target.empty()) {
          int r;
          
          lock.unlock();

          r = threads::pool::call(
            threads::PR_REQ_0, 
            bind(&symlink::internal_read, shared_from_this(), _1));

          if (r)
            return r;

          lock.lock();
        }

        *target = _target;
        return 0;
      }

      inline void set_target(const std::string &target)
      {
        boost::mutex::scoped_lock lock(_mutex);

        _target = target;
      }

    protected:
      virtual void set_request_body(const boost::shared_ptr<base::request> &req);

    private:
      int internal_read(const boost::shared_ptr<base::request> &req);

      boost::mutex _mutex;
      std::string _target;
    };
  }
}

#endif
