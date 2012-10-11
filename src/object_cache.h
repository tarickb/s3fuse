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
#include "objects/object.h"
#include "threads/thread_pool.h"

// TODO: move this into objects/ ?

namespace s3
{
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
    typedef boost::function1<void, objects::object::ptr> locked_object_function;

    static void init();
    static void print_summary();

    inline static objects::object::ptr get(const std::string &path, int hints = HINT_NONE)
    {
      objects::object::ptr obj = find(path);

      if (!obj)
        threads::thread_pool::call(
          threads::PR_REQ_0,
          boost::bind(&object_cache::fetch, _1, path, hints, &obj));

      return obj;
    }

    inline static objects::object::ptr get(const boost::shared_ptr<request> &req, const std::string &path, int hints = HINT_NONE)
    {
      objects::object::ptr obj = find(path);

      if (!obj)
        fetch(req, path, hints, &obj);

      return obj;
    }

    inline static void remove(const std::string &path)
    {
      boost::mutex::scoped_lock lock(s_mutex);

      s_cache_map.erase(path);
    }

    // this method is intended to ensure that fn() is called on the one and
    // only cached object at "path"
    inline static void lock_object(const std::string &path, const locked_object_function &fn)
    {
      boost::mutex::scoped_lock lock(s_mutex, boost::defer_lock);
      objects::object::ptr obj;

      // this puts the object at "path" in the cache if it isn't already there
      get(path);

      // but we do the following anyway so that we pass fn() whatever happens
      // to be in the cache.  it'll catch the (clearly pathological) case 
      // where:
      // 
      //   1. get(path) puts the object in the cache
      //   2. lock.lock() takes longer than the object expiry time (or some
      //      other delay occurs)
      //   3. some other, concurrent call to get(path) replaces the object
      //      in the cache
      //
      // of course it's possible that _cache_map[path] would have been pruned
      // before we can call fn(), but then we'd be returning an empty object
      // pointer, which fn() has to check for anyway.

      lock.lock();
      obj = s_cache_map[path];

      fn(obj);
    }

  private:
    typedef std::map<std::string, objects::object::ptr> cache_map;

    inline static objects::object::ptr find(const std::string &path)
    {
      boost::mutex::scoped_lock lock(s_mutex);
      objects::object::ptr &obj = s_cache_map[path];

      if (!obj) {
        s_misses++;

      } else if (obj->is_expired()) {
        s_expiries++;
        obj.reset();

      } else {
        s_hits++;
      }

      return obj;
    }

    static int fetch(const boost::shared_ptr<request> &req, const std::string &path, int hints, objects::object::ptr *obj);

    // TODO: prune periodically?

    static boost::mutex s_mutex;
    static cache_map s_cache_map;
    static uint64_t s_hits, s_misses, s_expiries;
  };
}

#endif
