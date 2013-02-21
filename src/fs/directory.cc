/*
 * fs/directory.cc
 * -------------------------------------------------------------------------
 * Directory class implementation.
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
#include "base/statistics.h"
#include "base/xml.h"
#include "fs/cache.h"
#include "fs/directory.h"
#include "services/service.h"
#include "threads/pool.h"

using boost::mutex;
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
using s3::fs::cache;
using s3::fs::directory;
using s3::fs::object;
using s3::services::service;
using s3::threads::pool;
using s3::threads::wait_async_handle;

namespace
{
  const char *IS_TRUNCATED_XPATH = "/ListBucketResult/IsTruncated";
  const char *         KEY_XPATH = "/ListBucketResult/Contents/Key";
  const char * NEXT_MARKER_XPATH = "/ListBucketResult/NextMarker";
  const char *      PREFIX_XPATH = "/ListBucketResult/CommonPrefixes/Prefix";

  struct rename_operation
  {
    shared_ptr<string> old_name;
    wait_async_handle::ptr handle;
  };

  atomic_count s_internal_objects_skipped_in_list(0);

  void statistics_writer(ostream *o)
  {
    *o << 
      "directories:\n"
      "  internal objects skipped in list: " << s_internal_objects_skipped_in_list << "\n";
  }

  int check_if_truncated(const xml::document &doc, bool *truncated)
  {
    int r;
    string temp;

    r = xml::find(doc, IS_TRUNCATED_XPATH, &temp);

    if (r)
      return r;

    *truncated = (temp == "true");
    return 0;
  }

  int precache_object(const request::ptr &req, const string &path, int hints)
  {
    // we need to wrap cache::get because it doesn't return anything, and the
    // thread pool expects a function that returns an int

    cache::get(req, path, hints);

    return 0;
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

  string marker = "";
  bool truncated = true;

  req->init(base::HTTP_GET);

  while (truncated) {
    xml::document doc;
    xml::element_list keys;

    req->set_url(service::get_bucket_url(), string("delimiter=/&prefix=") + request::url_encode(PREFIX) + "&marker=" + marker);
    req->run();

    if (req->get_response_code() != base::HTTP_SC_OK)
      throw runtime_error("failed to list objects.");

    doc = xml::parse(req->get_output_string());

    if (!doc)
      throw runtime_error("failed to parse object list.");

    if (check_if_truncated(doc, &truncated))
      throw runtime_error("list truncation check failed.");

    if (truncated && xml::find(doc, NEXT_MARKER_XPATH, &marker))
      throw runtime_error("unable to find list marker.");

    if (xml::find(doc, KEY_XPATH, &keys))
      throw runtime_error("unable to find list keys.");

    for (xml::element_list::const_iterator itor = keys.begin(); itor != keys.end(); ++itor)
      objects->push_back(itor->substr(PREFIX.size()));
  }
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
  string marker = "";
  string path = get_path();
  size_t path_len;
  bool truncated = true;
  cache_list_ptr cache;

  if (!path.empty())
    path += "/";

  path_len = path.size();

  if (config::get_cache_directories())
    cache.reset(new cache_list());

  req->init(base::HTTP_GET);

  while (truncated) {
    int r;
    xml::document doc;
    xml::element_list prefixes, keys;

    req->set_url(service::get_bucket_url(), string("delimiter=/&prefix=") + request::url_encode(path) + "&marker=" + marker);
    req->run();

    if (req->get_response_code() != base::HTTP_SC_OK)
      return -EIO;

    doc = xml::parse(req->get_output_string());

    if (!doc) {
      S3_LOG(LOG_WARNING, "directory::read", "failed to parse response.\n");
      return -EIO;
    }

    if ((r = check_if_truncated(doc, &truncated)))
      return r;

    if (truncated && (r = xml::find(doc, NEXT_MARKER_XPATH, &marker)))
      return r;

    if ((r = xml::find(doc, PREFIX_XPATH, &prefixes)))
      return r;

    if ((r = xml::find(doc, KEY_XPATH, &keys)))
      return r;

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

  if (cache) {
    mutex::scoped_lock lock(_mutex);

    _cache = cache;
  }

  return 0;
}

bool directory::is_empty(const request::ptr &req)
{
  xml::document doc;
  xml::element_list keys;

  // root directory isn't removable
  if (get_path().empty())
    return false;

  req->init(base::HTTP_GET);

  // set max-keys to two because GET will always return the path we request
  // note the trailing slash on path
  req->set_url(service::get_bucket_url(), string("prefix=") + request::url_encode(get_path()) + "/&max-keys=2");
  req->run();

  // if the request fails, assume the directory's not empty
  if (req->get_response_code() != base::HTTP_SC_OK)
    return false;

  doc = xml::parse(req->get_output_string());

  if (!doc) {
    S3_LOG(LOG_WARNING, "directory::is_empty", "failed to parse response.\n");
    return false;
  }

  if (xml::find(doc, KEY_XPATH, &keys))
    return false;

  return (keys.size() == 1);
}

int directory::remove(const request::ptr &req)
{
  if (!is_empty(req))
    return -ENOTEMPTY;

  return object::remove(req);
}

int directory::rename(const request::ptr &req, const string &to_)
{
  string from, to;
  size_t from_len;
  string marker = "";
  bool truncated = true;
  list<rename_operation> pending_renames, pending_deletes;

  // can't do anything with the root directory
  if (get_path().empty())
    return -EINVAL;

  from = get_path() + "/";
  to = to_ + "/";
  from_len = from.size();

  req->init(base::HTTP_GET);

  while (truncated) {
    xml::document doc;
    xml::element_list keys;
    int r;

    req->set_url(service::get_bucket_url(), string("prefix=") + request::url_encode(from) + "&marker=" + marker);
    req->run();

    if (req->get_response_code() != base::HTTP_SC_OK)
      return -EIO;

    doc = xml::parse(req->get_output_string());

    if (!doc) {
      S3_LOG(LOG_WARNING, "directory::rename", "failed to parse response.\n");
      return -EIO;
    }

    if ((r = check_if_truncated(doc, &truncated)))
      return r;

    if (truncated && (r = xml::find(doc, NEXT_MARKER_XPATH, &marker)))
      return r;

    if ((r = xml::find(doc, KEY_XPATH, &keys)))
      return r;

    for (xml::element_list::const_iterator itor = keys.begin(); itor != keys.end(); ++itor) {
      rename_operation oper;
      const char *full_path_cs = itor->c_str();
      const char *relative_path_cs = full_path_cs + from_len;
      string new_name = to + relative_path_cs;

      oper.old_name.reset(new string(full_path_cs));

      cache::remove(*oper.old_name);
      oper.handle = pool::post(threads::PR_REQ_1, bind(&object::copy_by_path, _1, *oper.old_name, new_name));

      pending_renames.push_back(oper);

      S3_LOG(LOG_DEBUG, "directory::rename", "[%s] -> [%s]\n", full_path_cs, new_name.c_str());
    }
  }

  while (!pending_renames.empty()) {
    int r;
    rename_operation &oper = pending_renames.front();

    r = oper.handle->wait();

    if (r)
      return r;

    oper.handle.reset();
    pending_deletes.push_back(oper);
    pending_renames.pop_front();
  }

  for (list<rename_operation>::iterator itor = pending_deletes.begin(); itor != pending_deletes.end(); ++itor)
    itor->handle = pool::post(threads::PR_REQ_1, bind(&object::remove_by_url, _1, object::build_url(*itor->old_name)));

  while (!pending_deletes.empty()) {
    int r;
    const rename_operation &oper = pending_deletes.front();

    r = oper.handle->wait();
    pending_deletes.pop_front();

    if (r)
      return r;
  }

  return 0;
}
