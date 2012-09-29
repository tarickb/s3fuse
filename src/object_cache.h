/*
 * object_cache.h
 * -------------------------------------------------------------------------
 * Caches object (i.e., file, directory, symlink) metadata.
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

#ifndef S3_OBJECT_CACHE_H
#define S3_OBJECT_CACHE_H

#include <map>
#include <string>
#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>

#include "logger.h"
#include "object.h"
#include "thread_pool.h"

namespace s3
{
  class file_transfer;
  class request;

  enum cache_hints
  {
    HINT_NONE    = 0x0,
    HINT_IS_DIR  = 0x1,
    HINT_IS_FILE = 0x2
  };

  class object_cache
  {
  public:
    typedef boost::shared_ptr<object_cache> ptr;

    object_cache(const thread_pool::ptr &pool);
    ~object_cache();

    inline object::ptr get(const std::string &path, int hints = HINT_NONE)
    {
      object::ptr obj = find(path);

      if (!obj)
        _pool->call(boost::bind(&object_cache::__fetch, this, _1, path, hints, &obj));

      return obj;
    }

    inline object::ptr get(const boost::shared_ptr<request> &req, const std::string &path, int hints = HINT_NONE)
    {
      object::ptr obj = find(path);

      if (!obj)
        __fetch(req, path, hints, &obj);

      return obj;
    }

    inline void remove(const std::string &path)
    {
      boost::mutex::scoped_lock lock(_mutex);

      _cache_map.erase(path);
    }

  private:
    typedef std::map<std::string, object::ptr> cache_map;

    inline object::ptr find(const std::string &path)
    {
      boost::mutex::scoped_lock lock(_mutex);
      object::ptr &obj = _cache_map[path];

      if (!obj) {
        _misses++;

      } else if (!obj->is_valid()) {
        _expiries++;
        obj.reset();

      } else {
        _hits++;
      }

      return obj;
    }

    int __fetch(const request_ptr &req, const std::string &path, int hints, object::ptr *obj);

    boost::mutex _mutex;
    cache_map _cache_map;
    thread_pool::ptr _pool;
    uint64_t _hits, _misses, _expiries;
  };
}

#endif
