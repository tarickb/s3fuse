/*
 * cache.cc
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

#include "logger.h"
#include "request.h"
#include "objects/cache.h"
#include "objects/directory.h"

using boost::mutex;
using std::string;

using s3::objects::cache;

boost::mutex cache::s_mutex;
cache::cache_map cache::s_cache_map;
uint64_t cache::s_hits, cache::s_misses, cache::s_expiries;

void cache::init()
{
  s_hits = 0;
  s_misses = 0;
  s_expiries = 0;
}

void cache::print_summary()
{
  uint64_t total = s_hits + s_misses + s_expiries;

  if (total == 0)
    total = 1; // avoid NaNs below

  S3_LOG(
    LOG_DEBUG,
    "cache::print_summary", 
    "hits: %" PRIu64 " (%.02f%%), misses: %" PRIu64 " (%.02f%%), expiries: %" PRIu64 " (%.02f%%)\n", 
    s_hits,
    double(s_hits) / double(total) * 100.0,
    s_misses,
    double(s_misses) / double(total) * 100.0,
    s_expiries,
    double(s_expiries) / double(total) * 100.0);
}

int cache::fetch(const request::ptr &req, const string &path, int hints, object::ptr *obj)
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
