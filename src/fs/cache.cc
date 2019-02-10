/*
 * fs/cache.cc
 * -------------------------------------------------------------------------
 * Object fetching (metadata only).
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

#include "fs/cache.h"

#include <atomic>
#include <mutex>

#include "base/config.h"
#include "base/logger.h"
#include "base/lru_cache_map.h"
#include "base/request.h"
#include "base/statistics.h"
#include "fs/directory.h"
#include "fs/object.h"
#include "threads/pool.h"

namespace s3 {
namespace fs {
namespace {
inline bool IsObjectRemovable(const std::shared_ptr<Object> &obj) {
  return !obj || obj->IsRemovable();
}

std::mutex s_mutex;
std::unique_ptr<
    base::LruCacheMap<std::string, std::shared_ptr<Object>, IsObjectRemovable>>
    s_cache_map;
std::atomic_int s_get_failures(0);
uint64_t s_hits = 0, s_misses = 0, s_expiries = 0;

int Fetch(base::Request *req, const std::string &path, CacheHints hints,
          std::shared_ptr<Object> *obj) {
  if (!path.empty()) {
    req->Init(base::HttpMethod::HEAD);

    if ((hints == CacheHints::NONE || hints == CacheHints::IS_DIR) &&
        !Object::IsVersionedPath(path)) {
      // see if the path is a directory (trailing /) first
      req->SetUrl(Directory::BuildUrl(path));
      req->Run();
    }

    if (hints == CacheHints::IS_FILE ||
        req->response_code() != base::HTTP_SC_OK) {
      // it's not a directory
      req->SetUrl(Object::BuildUrl(path));
      req->Run();
    }

    if (req->response_code() != base::HTTP_SC_OK) {
      ++s_get_failures;
      return 0;
    }
  }

  *obj = Object::Create(path, req);

  {
    std::lock_guard<std::mutex> lock(s_mutex);
    auto &map_obj = (*s_cache_map)[path];
    if (map_obj) {
      // if the object is already in the map, don't overwrite it
      *obj = map_obj;
    } else {
      // otherwise, save it
      map_obj = *obj;
    }
  }

  return 0;
}

inline double Percent(uint64_t a, uint64_t b) {
  return static_cast<double>(a) / static_cast<double>(b) * 100.0;
}

void StatsWriter(std::ostream *o) {
  uint64_t total = s_hits + s_misses + s_expiries;
  if (total == 0) total = 1;  // avoid NaNs below
  o->setf(std::ostream::fixed);
  o->precision(2);
  *o << "object cache:\n"
        "  size: "
     << s_cache_map->size()
     << "\n"
        "  hits: "
     << s_hits << " (" << Percent(s_hits, total)
     << " %)\n"
        "  misses: "
     << s_misses << " (" << Percent(s_misses, total)
     << " %)\n"
        "  expiries: "
     << s_expiries << " (" << Percent(s_expiries, total)
     << " %)\n"
        "  get failures: "
     << s_get_failures << "\n";
}

base::Statistics::Writers::Entry s_writer(StatsWriter, 0);
}  // namespace

void Cache::Init() {
  s_cache_map.reset(new base::LruCacheMap<std::string, std::shared_ptr<Object>,
                                          IsObjectRemovable>(
      base::Config::max_objects_in_cache()));
}
std::shared_ptr<Object> Cache::Get(const std::string &path, CacheHints hints) {
  std::shared_ptr<Object> obj;
  {
    std::lock_guard<std::mutex> lock(s_mutex);
    s_cache_map->Find(path, &obj);
    if (!obj) {
      ++s_misses;
    } else if (obj->expired() && obj->IsRemovable()) {
      ++s_expiries;
      s_cache_map->Erase(path);
    } else {
      s_hits++;
    }
  }
  if (!obj) {
    threads::Pool::Call(
        threads::PoolId::PR_REQ_0,
        std::bind(&Fetch, std::placeholders::_1, path, hints, &obj));
  }
  return obj;
}

int Cache::Preload(base::Request *req, const std::string &path,
                   CacheHints hints) {
  std::lock_guard<std::mutex> lock(s_mutex);
  if (!s_cache_map->Find(path, nullptr)) Fetch(req, path, hints, nullptr);
  return 0;
}

int Cache::Remove(const std::string &path) {
  std::lock_guard<std::mutex> lock(s_mutex);
  std::shared_ptr<Object> o;
  if (!s_cache_map->Find(path, &o)) return 0;
  if (!o->IsRemovable()) return -EBUSY;
  s_cache_map->Erase(path);
  return 0;
}

void Cache::LockObject(const std::string &path,
                       const LockedObjectCallback &callback) {
  std::unique_lock<std::mutex> lock(s_mutex, std::defer_lock);
  std::shared_ptr<Object> obj;

  // this puts the object at "path" in the cache if it isn't already there
  Get(path);

  // but we do the following anyway so that we pass callback() whatever happens
  // to be in the cache.  it'll catch the (clearly pathological) case
  // where:
  //
  //   1. Get(path) puts the object in the cache
  //   2. lock.lock() takes longer than the object expiry time (or some
  //      other delay occurs)
  //   3. some other, concurrent call to Get(path) replaces the object
  //      in the cache
  //
  // of course it's possible that s_cache_map[path] would have been pruned
  // before we can call callback(), but then we'd be returning an empty object
  // pointer, which callback() has to check for anyway.

  lock.lock();
  obj = (*s_cache_map)[path];
  callback(obj);
}
}  // namespace fs
}  // namespace s3
