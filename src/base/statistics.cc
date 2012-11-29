/*
 * base/statistics.cc
 * -------------------------------------------------------------------------
 * Definitions for statistics-collection methods.
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

#include <map>
#include <boost/thread.hpp>

#include "base/statistics.h"

using boost::mutex;
using std::endl;
using std::make_pair;
using std::multimap;
using std::ostream;
using std::string;

using s3::base::statistics;

namespace
{
  typedef multimap<string, string> stat_map;

  stat_map *s_map = NULL;
  mutex *s_mutex = NULL;
}

void statistics::init()
{
  s_map = new stat_map();
  s_mutex = new mutex();
}

void statistics::write(ostream *o)
{
  if (s_map && s_mutex) {
    mutex::scoped_lock lock(*s_mutex);

    for (stat_map::const_iterator itor = s_map->begin(); itor != s_map->end(); ++itor)
      *o << itor->first << ":" << itor->second << endl;
  }
}

void statistics::post(const string &id, const string &stats)
{
  if (s_map && s_mutex) {
    mutex::scoped_lock lock(*s_mutex);

    s_map->insert(make_pair(id, stats));
  }
}
