/*
 * fs/cache.h
 * -------------------------------------------------------------------------
 * Caches object (i.e., file, directory, symlink) metadata.
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

#ifndef S3_FS_CACHE_H
#define S3_FS_CACHE_H

#include <functional>
#include <memory>
#include <mutex>
#include <string>

#include "base/logger.h"
#include "base/lru_cache_map.h"
#include "base/statistics.h"
#include "fs/object.h"
#include "threads/pool.h"

namespace s3 {
namespace base {
class request;
}

namespace fs {
enum cache_hints { HINT_NONE = 0x0, HINT_IS_DIR = 0x1, HINT_IS_FILE = 0x2 };

class cache {
public:
  typedef std::function<void(object::ptr)> locked_object_function;

  static void init();

  inline static object::ptr get(const std::string &path,
                                int hints = HINT_NONE) {
    object::ptr obj = find(path);

    if (!obj)
      threads::pool::call(
          threads::PR_REQ_0,
          std::bind(&cache::fetch, std::placeholders::_1, path, hints, &obj));

    return obj;
  }

  inline static object::ptr get(const std::shared_ptr<base::request> &req,
                                const std::string &path,
                                int hints = HINT_NONE) {
    object::ptr obj = find(path);

    if (!obj)
      fetch(req, path, hints, &obj);

    return obj;
  }

  inline static int remove(const std::string &path) {
    std::lock_guard<std::mutex> lock(s_mutex);
    object::ptr o;

    if (!s_cache_map->find(path, &o))
      return 0;

    if (!o->is_removable())
      return -EBUSY;

    s_cache_map->erase(path);

    return 0;
  }

  // this method is intended to ensure that fn() is called on the one and
  // only cached object at "path"
  inline static void lock_object(const std::string &path,
                                 const locked_object_function &fn) {
    std::unique_lock<std::mutex> lock(s_mutex, std::defer_lock);
    object::ptr obj;

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
    obj = (*s_cache_map)[path];

    fn(obj);
  }

private:
  inline static bool is_object_removable(const object::ptr &obj) {
    return !obj || obj->is_removable();
  }

  inline static object::ptr find(const std::string &path) {
    std::lock_guard<std::mutex> lock(s_mutex);
    object::ptr &obj = (*s_cache_map)[path];

    if (!obj) {
      s_misses++;

    } else if (obj->is_expired() && obj->is_removable()) {
      s_expiries++;
      obj.reset();

    } else {
      s_hits++;
    }

    return obj;
  }

  static void statistics_writer(std::ostream *o);
  static int fetch(const std::shared_ptr<base::request> &req,
                   const std::string &path, int hints, object::ptr *obj);

  typedef base::lru_cache_map<std::string, object::ptr, is_object_removable>
      cache_map;

  static std::mutex s_mutex;
  static std::unique_ptr<cache_map> s_cache_map;
  static uint64_t s_hits, s_misses, s_expiries;

  static base::statistics::writers::entry s_writer;
};
} // namespace fs
} // namespace s3

#endif
