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

#include <atomic>

#include "base/config.h"
#include "base/logger.h"
#include "base/request.h"
#include "base/statistics.h"
#include "base/xml.h"
#include "fs/cache.h"
#include "fs/directory.h"
#include "fs/list_reader.h"
#include "threads/parallel_work_queue.h"
#include "threads/pool.h"

namespace s3 {
namespace fs {

namespace {
std::atomic_int s_internal_objects_skipped_in_list(0);
std::atomic_int s_copy_retries(0), s_delete_retries(0);

void statistics_writer(std::ostream *o) {
  *o << "directories:\n"
        "  internal objects skipped in list: "
     << s_internal_objects_skipped_in_list
     << "\n"
        "  rename retries (copy step): "
     << s_copy_retries
     << "\n"
        "  rename retries (delete step): "
     << s_delete_retries << "\n";
}

int precache_object(const base::request::ptr &req, const std::string &path,
                    int hints) {
  // we need to wrap cache::get because it returns an object::ptr, and the
  // thread pool expects a function that returns an int

  cache::get(req, path, hints);

  return 0;
}

int copy_object(const base::request::ptr &req, std::string *name,
                const std::string &old_base, const std::string &new_base,
                bool is_retry) {
  std::string old_name = old_base + *name;
  std::string new_name = new_base + *name;

  if (is_retry)
    ++s_copy_retries;

  S3_LOG(LOG_DEBUG, "directory::copy_object", "[%s] -> [%s]\n",
         old_name.c_str(), new_name.c_str());

  return object::copy_by_path(req, old_name, new_name);
}

int delete_object(const base::request::ptr &req, std::string *name,
                  const std::string &old_base, bool is_retry) {
  std::string old_name = old_base + *name;

  if (is_retry)
    ++s_delete_retries;

  S3_LOG(LOG_DEBUG, "directory::delete_object", "[%s]\n", old_name.c_str());

  return object::remove_by_url(req, object::build_url(old_name));
}

object *checker(const std::string &path, const base::request::ptr &req) {
  const std::string &url = req->get_url();

  if (!path.empty() && (url.empty() || url[url.size() - 1] != '/'))
    return NULL;

  return new directory(path);
}

object::type_checker_list::entry s_checker_reg(checker, 10);
base::statistics::writers::entry s_writer(statistics_writer, 0);
} // namespace

std::string directory::build_url(const std::string &path) {
  return object::build_url(path) + "/";
}

void directory::get_internal_objects(const base::request::ptr &req,
                                     std::vector<std::string> *objects) {
  const std::string &PREFIX = object::get_internal_prefix();

  list_reader::ptr reader;
  base::xml::element_list keys;
  int r;

  reader.reset(new list_reader(PREFIX));

  while ((r = reader->read(req, &keys, NULL)) > 0) {
    for (base::xml::element_list::const_iterator itor = keys.begin();
         itor != keys.end(); ++itor)
      objects->push_back(itor->substr(PREFIX.size()));
  }

  if (r)
    throw std::runtime_error("failed to list bucket objects.");
}

directory::directory(const std::string &path) : object(path) {
  set_url(build_url(path));
  set_type(S_IFDIR);
}

directory::~directory() {}

int directory::do_read(const base::request::ptr &req,
                       const filler_function &filler) {
  std::string path = get_path();
  size_t path_len;
  cache_list_ptr cache;
  list_reader::ptr reader;
  base::xml::element_list prefixes, keys;
  int r;

  if (!path.empty())
    path += "/";

  path_len = path.size();

  if (base::config::get_cache_directories())
    cache.reset(new cache_list());

  reader.reset(new list_reader(path));

  // for POSIX compliance
  filler(".");
  filler("..");

  while ((r = reader->read(req, &keys, &prefixes)) > 0) {
    for (base::xml::element_list::const_iterator itor = prefixes.begin();
         itor != prefixes.end(); ++itor) {
      // strip trailing slash
      std::string relative_path =
          itor->substr(path_len, itor->size() - path_len - 1);

      filler(relative_path);

      if (base::config::get_precache_on_readdir())
        threads::pool::call_async(threads::PR_REQ_1,
                                  bind(precache_object, std::placeholders::_1,
                                       path + relative_path, HINT_IS_DIR));

      if (cache)
        cache->push_back(relative_path);
    }

    for (base::xml::element_list::const_iterator itor = keys.begin();
         itor != keys.end(); ++itor) {
      if (path != *itor) {
        std::string relative_path = itor->substr(path_len);

        if (object::is_internal_path(relative_path)) {
          ++s_internal_objects_skipped_in_list;
          continue;
        }

        filler(relative_path);

        if (base::config::get_precache_on_readdir())
          threads::pool::call_async(threads::PR_REQ_1,
                                    bind(precache_object, std::placeholders::_1,
                                         path + relative_path, HINT_IS_FILE));

        if (cache)
          cache->push_back(relative_path);
      }
    }
  }

  if (r)
    return r;

  if (cache) {
    std::lock_guard<std::mutex> lock(_mutex);

    _cache = cache;
  }

  return 0;
}

bool directory::is_empty(const base::request::ptr &req) {
  list_reader::ptr reader;
  base::xml::element_list keys;

  // root directory isn't removable
  if (get_path().empty())
    return false;

  // set max_keys to two because GET will always return the path we request
  reader.reset(new list_reader(get_path() + "/", false, 2));

  return (reader->read(req, &keys, NULL) == 1);
}

int directory::remove(const base::request::ptr &req) {
  if (!is_empty(req))
    return -ENOTEMPTY;

  return object::remove(req);
}

int directory::rename(const base::request::ptr &req, const std::string &to_) {
  typedef threads::parallel_work_queue<std::string> rename_queue;

  std::string from, to;
  size_t from_len;
  list_reader::ptr reader;
  base::xml::element_list keys;
  std::list<std::string> relative_paths;
  std::unique_ptr<rename_queue> queue;
  int r;

  // can't do anything with the root directory
  if (get_path().empty())
    return -EINVAL;

  from = get_path() + "/";
  to = to_ + "/";
  from_len = from.size();

  reader.reset(new list_reader(from, false));

  cache::remove(get_path());

  while ((r = reader->read(req, &keys, NULL)) > 0) {
    for (base::xml::element_list::const_iterator itor = keys.begin();
         itor != keys.end(); ++itor) {
      int rm_r;

      if ((rm_r = cache::remove(*itor)))
        return rm_r;

      relative_paths.push_back(itor->substr(from_len));
    }
  }

  queue.reset(new rename_queue(relative_paths.begin(), relative_paths.end(),
                               bind(&copy_object, std::placeholders::_1,
                                    std::placeholders::_2, from, to, false),
                               bind(&copy_object, std::placeholders::_1,
                                    std::placeholders::_2, from, to, true)));

  if ((r = queue->process()))
    return r;

  queue.reset(new rename_queue(relative_paths.begin(), relative_paths.end(),
                               bind(&delete_object, std::placeholders::_1,
                                    std::placeholders::_2, from, false),
                               bind(&delete_object, std::placeholders::_1,
                                    std::placeholders::_2, from, true)));

  return queue->process();
}

} // namespace fs
} // namespace s3
