/*
 * object_cache.cc
 * -------------------------------------------------------------------------
 * Object fetching (metadata only).
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

#include "object_builder.h"
#include "object_cache.h"
#include "request.h"

using namespace boost;
using namespace std;

using namespace s3;

object_cache::object_cache(const thread_pool::ptr &pool)
  : _pool(pool),
    _hits(0),
    _misses(0),
    _expiries(0),
    _expired_but_unremovable(0)
{ }

object_cache::~object_cache()
{
  // don't add expired-but-unremovable since they count as hits too
  uint64_t total = _hits + _misses + _expiries;

  if (total == 0)
    total = 1; // avoid NaNs below

  S3_LOG(
    LOG_DEBUG,
    "object_cache::~object_cache", 
    "hits: %" PRIu64 " (%.02f%%), misses: %" PRIu64 " (%.02f%%), expiries: %" PRIu64 " (%.02f%%), expired but unremovable: %" PRIu64 " (%.02f%%)\n", 
    _hits,
    double(_hits) / double(total) * 100.0,
    _misses,
    double(_misses) / double(total) * 100.0,
    _expiries,
    double(_expiries) / double(total) * 100.0,
    _expired_but_unremovable,
    double(_expired_but_unremovable) / double(total) * 100.0);
}

int object_cache::__fetch(const request::ptr &req, const string &path, object_type type_hint, object::ptr *obj_, locked_object::ptr *l_)
{
  object::ptr obj;
  object_builder builder(req, path, type_hint);

  obj = builder.build();

  {
    mutex::scoped_lock lock(_mutex);
    object::ptr &map_obj = _cache_map[path];

    if (map_obj) {
      // if the object is already in the map, don't overwrite it
      obj = map_obj;
    } else {
      // otherwise, save it
      map_obj = obj;
    }

    if (*obj_)
      *obj_ = obj;

    if (*l_)
      *l_ = locked_object::ptr(new locked_object(obj));
  }

  return 0;
}
