/*
 * fs/directory.h
 * -------------------------------------------------------------------------
 * Represents a directory object.
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

#ifndef S3_FS_DIRECTORY_H
#define S3_FS_DIRECTORY_H

#include "fs/object.h"
#include "threads/pool.h"

namespace s3
{
  namespace fs
  {
    class directory : public object
    {
    public:
      typedef boost::shared_ptr<directory> ptr;
      typedef boost::function1<void, const std::string &> filler_function;

      static std::string build_url(const std::string &path);
      static void invalidate_parent(const std::string &path);
      static void get_internal_objects(const boost::shared_ptr<base::request> &req, std::vector<std::string> *objects);

      directory(const std::string &path);
      virtual ~directory();

      inline ptr shared_from_this()
      {
        return boost::static_pointer_cast<directory>(object::shared_from_this());
      }

      inline int read(const filler_function &filler)
      {
        boost::mutex::scoped_lock lock(_mutex);
        cache_list_ptr cache;

        cache = _cache;
        lock.unlock();

        if (cache) {
          // for POSIX compliance
          filler(".");
          filler("..");

          for (cache_list::const_iterator itor = cache->begin(); itor != cache->end(); ++itor)
            filler(*itor);

          return 0;
        } else {
          return threads::pool::call(
            threads::PR_REQ_0, 
            bind(&directory::read, shared_from_this(), _1, filler));
        }
      }

      bool is_empty(const boost::shared_ptr<base::request> &req);

      inline bool is_empty()
      {
        return threads::pool::call(
          threads::PR_REQ_0, 
          boost::bind(&directory::is_empty, shared_from_this(), _1));
      }

      virtual int remove(const boost::shared_ptr<base::request> &req);
      virtual int rename(const boost::shared_ptr<base::request> &req, const std::string &to);

    private:
      typedef std::list<std::string> cache_list;
      typedef boost::shared_ptr<cache_list> cache_list_ptr;

      int read(const boost::shared_ptr<base::request> &req, const filler_function &filler);

      boost::mutex _mutex;
      cache_list_ptr _cache;
    };
  }
}

#endif
