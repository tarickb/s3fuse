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

#include "directory.h"
#include "object_cache.h"
#include "request.h"

using namespace boost;
using namespace std;

using namespace s3;

boost::mutex object_cache::s_mutex;
object_cache::cache_map object_cache::s_cache_map;
uint64_t object_cache::s_hits, object_cache::s_misses, object_cache::s_expiries;

void object_cache::init()
{
  s_hits = 0;
  s_misses = 0;
  s_expiries = 0;
}

void object_cache::print_summary()
{
  uint64_t total = s_hits + s_misses + s_expiries;

  if (total == 0)
    total = 1; // avoid NaNs below

  S3_LOG(
    LOG_DEBUG,
    "object_cache::print_summary", 
    "hits: %" PRIu64 " (%.02f%%), misses: %" PRIu64 " (%.02f%%), expiries: %" PRIu64 " (%.02f%%)\n", 
    s_hits,
    double(s_hits) / double(total) * 100.0,
    s_misses,
    double(s_misses) / double(total) * 100.0,
    s_expiries,
    double(s_expiries) / double(total) * 100.0);
}

int object_cache::fetch(const request::ptr &req, const string &path, int hints, object::ptr *obj)
{
  if (!path.empty()) {
    req->init(HTTP_HEAD);

    if (hints == HINT_NONE || hints & HINT_IS_DIR) {
      // see if the path is a directory (trailing /) first
      req->set_url(directory::build_url(path));
      req->run();
    }

    if (hints & HINT_IS_FILE || req->get_response_code() != HTTP_SC_OK) {
      // it's not a directory
      req->set_url(object::build_url(path));
      req->run();
    }

    if (req->get_response_code() != HTTP_SC_OK)
      return 0;
  }

  *obj = object::create(path, req);

  {
    mutex::scoped_lock lock(s_mutex);
    object::ptr &map_obj = s_cache_map[path];

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
