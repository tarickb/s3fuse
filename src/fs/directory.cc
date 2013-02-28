/*
 * fs/directory.cc
 * -------------------------------------------------------------------------
 * Directory class implementation.
 * -------------------------------------------------------------------------
 *
 * Copyright (c) 2013, Tarick Bedeir.
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
#include "base/statistics.h"
#include "base/xml.h"
#include "fs/bucket_reader.h"
#include "fs/cache.h"
#include "fs/directory.h"
#include "threads/parallel_work_queue.h"
#include "threads/pool.h"

using boost::mutex;
using boost::scoped_ptr;
using boost::shared_ptr;
using boost::detail::atomic_count;
using std::list;
using std::ostream;
using std::runtime_error;
using std::string;
using std::vector;

using s3::base::config;
using s3::base::request;
using s3::base::statistics;
using s3::base::xml;
using s3::fs::bucket_reader;
using s3::fs::cache;
using s3::fs::directory;
using s3::fs::object;
using s3::threads::parallel_work_queue;
using s3::threads::pool;
using s3::threads::wait_async_handle;

namespace
{
  atomic_count s_internal_objects_skipped_in_list(0);
  atomic_count s_copy_retries(0), s_delete_retries(0);

  void statistics_writer(ostream *o)
  {
    *o << 
      "directories:\n"
      "  internal objects skipped in list: " << s_internal_objects_skipped_in_list << "\n"
      "  rename retries (copy step): " << s_copy_retries << "\n"
      "  rename retries (delete step): " << s_delete_retries << "\n";
  }

  int precache_object(const request::ptr &req, const string &path, int hints)
  {
    // we need to wrap cache::get because it returns an object::ptr, and the
    // thread pool expects a function that returns an int

    cache::get(req, path, hints);

    return 0;
  }

  int copy_object(const request::ptr &req, string *name, const string &old_base, const string &new_base, bool is_retry)
  {
    string old_name = old_base + *name;
    string new_name = new_base + *name;

    if (is_retry)
      ++s_copy_retries;

    S3_LOG(LOG_DEBUG, "directory::copy_object", "[%s] -> [%s]\n", old_name.c_str(), new_name.c_str());

    return object::copy_by_path(req, old_name, new_name);
  }

  int delete_object(const request::ptr &req, string *name, const string &old_base, bool is_retry)
  {
    string old_name = old_base + *name;

    if (is_retry)
      ++s_delete_retries;

    S3_LOG(LOG_DEBUG, "directory::delete_object", "[%s]\n", old_name.c_str());

    return object::remove_by_url(req, object::build_url(old_name));
  }

  object * checker(const string &path, const request::ptr &req)
  {
    const string &url = req->get_url();

    if (!path.empty() && (url.empty() || url[url.size() - 1] != '/'))
      return NULL;

    return new directory(path);
  }

  object::type_checker_list::entry s_checker_reg(checker, 10);
  statistics::writers::entry s_writer(statistics_writer, 0);
}

string directory::build_url(const string &path)
{
  return object::build_url(path) + "/";
}

void directory::invalidate_parent(const string &path)
{
  if (config::get_cache_directories()) {
    string parent_path;
    size_t last_slash = path.rfind('/');

    parent_path = (last_slash == string::npos) ? "" : path.substr(0, last_slash);

    S3_LOG(LOG_DEBUG, "directory::invalidate_parent", "invalidating parent directory [%s] for [%s].\n", parent_path.c_str(), path.c_str());
    cache::remove(parent_path);
  }
}

void directory::get_internal_objects(const request::ptr &req, vector<string> *objects)
{
  const string &PREFIX = object::get_internal_prefix();

  bucket_reader::ptr reader;
  xml::element_list keys;
  int r;

  reader.reset(new bucket_reader(PREFIX));

  while ((r = reader->read(req, &keys, NULL)) > 0) {
    for (xml::element_list::const_iterator itor = keys.begin(); itor != keys.end(); ++itor)
      objects->push_back(itor->substr(PREFIX.size()));
  }

  if (r)
    throw runtime_error("failed to list bucket objects.");
}

directory::directory(const string &path)
  : object(path)
{
  set_url(build_url(path));
  set_object_type(S_IFDIR);
}

directory::~directory()
{
}

int directory::read(const request::ptr &req, const filler_function &filler)
{
  string path = get_path();
  size_t path_len;
  cache_list_ptr cache;
  bucket_reader::ptr reader;
  xml::element_list prefixes, keys;
  int r;

  if (!path.empty())
    path += "/";

  path_len = path.size();

  if (config::get_cache_directories())
    cache.reset(new cache_list());

  reader.reset(new bucket_reader(path));

  while ((r = reader->read(req, &keys, &prefixes)) > 0) {
    for (xml::element_list::const_iterator itor = prefixes.begin(); itor != prefixes.end(); ++itor) {
      // strip trailing slash
      string relative_path = itor->substr(path_len, itor->size() - path_len - 1);

      filler(relative_path);

      if (config::get_precache_on_readdir())
        pool::call_async(threads::PR_REQ_1, bind(precache_object, _1, path + relative_path, HINT_IS_DIR));

      if (cache)
        cache->push_back(relative_path);
    }

    for (xml::element_list::const_iterator itor = keys.begin(); itor != keys.end(); ++itor) {
      if (path != *itor) {
        string relative_path = itor->substr(path_len);

        if (object::is_internal_path(relative_path)) {
          ++s_internal_objects_skipped_in_list;
          continue;
        }

        filler(relative_path);

        if (config::get_precache_on_readdir())
          pool::call_async(threads::PR_REQ_1, bind(precache_object, _1, path + relative_path, HINT_IS_FILE));

        if (cache)
          cache->push_back(relative_path);
      }
    }
  }

  if (r)
    return r;

  if (cache) {
    mutex::scoped_lock lock(_mutex);

    _cache = cache;
  }

  return 0;
}

bool directory::is_empty(const request::ptr &req)
{
  bucket_reader::ptr reader;
  xml::element_list keys;

  // root directory isn't removable
  if (get_path().empty())
    return false;

  // set max_keys to two because GET will always return the path we request
  reader.reset(new bucket_reader(get_path() + "/", false, 2));

  return (reader->read(req, &keys, NULL) == 1);
}

int directory::remove(const request::ptr &req)
{
  if (!is_empty(req))
    return -ENOTEMPTY;

  return object::remove(req);
}

int directory::rename(const request::ptr &req, const string &to_)
{
  typedef parallel_work_queue<string> rename_queue;

  string from, to;
  size_t from_len;
  bucket_reader::ptr reader;
  xml::element_list keys;
  list<string> relative_paths;
  scoped_ptr<rename_queue> queue;
  int r;

  // can't do anything with the root directory
  if (get_path().empty())
    return -EINVAL;

  from = get_path() + "/";
  to = to_ + "/";
  from_len = from.size();

  reader.reset(new bucket_reader(from, false));

  while ((r = reader->read(req, &keys, NULL)) > 0) {
    for (xml::element_list::const_iterator itor = keys.begin(); itor != keys.end(); ++itor) {
      int rm_r;

      if ((rm_r = cache::remove(*itor)))
        return rm_r;

      relative_paths.push_back(itor->substr(from_len));
    }
  }

  queue.reset(new rename_queue(
    relative_paths.begin(),
    relative_paths.end(),
    bind(&copy_object, _1, _2, from, to, false),
    bind(&copy_object, _1, _2, from, to, true)));

  if ((r = queue->process()))
    return r;

  queue.reset(new rename_queue(
    relative_paths.begin(),
    relative_paths.end(),
    bind(&delete_object, _1, _2, from, false),
    bind(&delete_object, _1, _2, from, true)));

  return queue->process();
}
