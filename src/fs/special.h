/*
 * fs/special.h
 * -------------------------------------------------------------------------
 * Represents a "special" object (e.g., FIFO, device, etc.).
 * -------------------------------------------------------------------------
 *
 * Copyright (c) 2013, Tarick Bedeir.
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

#ifndef S3_FS_SPECIAL_H
#define S3_FS_SPECIAL_H

#include <string>

#include "fs/object.h"

namespace s3
{
  namespace fs
  {
    class special : public object
    {
    public:
      typedef boost::shared_ptr<special> ptr;

      special(const std::string &path);
      virtual ~special();

      inline ptr shared_from_this()
      {
        return boost::static_pointer_cast<special>(object::shared_from_this());
      }

      inline void set_type(mode_t mode)
      {
        object::set_type(mode & S_IFMT);
      }

      inline void set_device(dev_t dev)
      {
        get_stat()->st_rdev = dev;
      }

    protected:
      virtual void init(const boost::shared_ptr<base::request> &req);
      virtual void set_request_headers(const boost::shared_ptr<base::request> &req);
    };
  }
}

#endif
