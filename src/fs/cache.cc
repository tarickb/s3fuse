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

#include <boost/detail/atomic_count.hpp>

#include "base/config.h"
#include "base/logger.h"
#include "base/request.h"
#include "fs/cache.h"
#include "fs/directory.h"

using boost::mutex;
using boost::scoped_ptr;
using boost::detail::atomic_count;
using std::ostream;
using std::string;

using s3::base::config;
using s3::base::request;
using s3::base::statistics;
using s3::fs::cache;

boost::mutex cache::s_mutex;
scoped_ptr<cache::cache_map> cache::s_cache_map;
uint64_t cache::s_hits(0), cache::s_misses(0), cache::s_expiries(0);
statistics::writers::entry cache::s_writer(cache::statistics_writer, 0);

namespace
{
  atomic_count s_get_failures(0);

  inline double percent(uint64_t a, uint64_t b)
  {
    return static_cast<double>(a) / static_cast<double>(b) * 100.0;
  }
}

void cache::init()
{
  s_cache_map.reset(new cache_map(config::get_max_objects_in_cache()));
}

void cache::statistics_writer(ostream *o)
{
  uint64_t total = s_hits + s_misses + s_expiries;

  if (total == 0)
    total = 1; // avoid NaNs below

  o->setf(ostream::fixed);
  o->precision(2);

  *o << 
    "object cache:\n"
    "  size: " << s_cache_map->get_size() << "\n"
    "  hits: " << s_hits << " (" << percent(s_hits, total) << " %)\n"
    "  misses: " << s_misses << " (" << percent(s_misses, total) << " %)\n"
    "  expiries: " << s_expiries << " (" << percent(s_expiries, total) << " %)\n"
    "  get failures: " << s_get_failures << "\n";
}

int cache::fetch(const request::ptr &req, const string &path, int hints, object::ptr *obj)
{
  if (!path.empty()) {
    req->init(base::HTTP_HEAD);

    if (hints == HINT_NONE || hints & HINT_IS_DIR) {
      // see if the path is a directory (trailing /) first
      req->set_url(directory::build_url(path));
      req->run();
    }

    if (hints & HINT_IS_FILE || req->get_response_code() != base::HTTP_SC_OK) {
      // it's not a directory
      req->set_url(object::build_url(path));
      req->run();
    }

    if (req->get_response_code() != base::HTTP_SC_OK) {
      ++s_get_failures;
      return 0;
    }
  }

  *obj = object::create(path, req);

  {
    mutex::scoped_lock lock(s_mutex);
    object::ptr &map_obj = (*s_cache_map)[path];

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
