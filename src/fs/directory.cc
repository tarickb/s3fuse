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

#include "fs/directory.h"

#include <atomic>

#include "base/config.h"
#include "base/logger.h"
#include "base/request.h"
#include "base/statistics.h"
#include "base/xml.h"
#include "fs/cache.h"
#include "fs/list_reader.h"
#include "threads/parallel_work_queue.h"
#include "threads/pool.h"

namespace s3 {
namespace fs {

namespace {
std::atomic_int s_internal_objects_skipped_in_list(0);
std::atomic_int s_copy_retries(0), s_delete_retries(0);

void StatsWriter(std::ostream *o) {
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

int CopyObject(base::Request *req, std::string *name,
               const std::string &old_base, const std::string &new_base,
               bool is_retry) {
  if (is_retry) ++s_copy_retries;
  const std::string old_name = old_base + *name;
  const std::string new_name = new_base + *name;
  S3_LOG(LOG_DEBUG, "Directory::CopyObject", "[%s] -> [%s]\n", old_name.c_str(),
         new_name.c_str());
  return Object::CopyByPath(req, old_name, new_name);
}

int DeleteObject(base::Request *req, std::string *name,
                 const std::string &old_base, bool is_retry) {
  if (is_retry) ++s_delete_retries;
  const std::string old_name = old_base + *name;
  S3_LOG(LOG_DEBUG, "Directory::DeleteObject", "[%s]\n", old_name.c_str());
  return Object::RemoveByUrl(req, Object::BuildUrl(old_name));
}

Object *Checker(const std::string &path, base::Request *req) {
  std::string url = req->url();
  if (!path.empty() && (url.empty() || url[url.size() - 1] != '/'))
    return nullptr;
  return new Directory(path);
}

Object::TypeCheckers::Entry s_checker_reg(Checker, 10);
base::Statistics::Writers::Entry s_writer(StatsWriter, 0);
}  // namespace

std::string Directory::BuildUrl(const std::string &path) {
  return Object::BuildUrl(path) + "/";
}

std::vector<std::string> Directory::GetInternalObjects(base::Request *req) {
  const std::string prefix = Object::internal_prefix();
  ListReader reader(prefix);
  std::list<std::string> keys;
  std::vector<std::string> objects;
  int r;
  while ((r = reader.Read(req, &keys, nullptr)) > 0) {
    for (const auto &key : keys) objects.push_back(key.substr(prefix.size()));
  }
  if (r) throw std::runtime_error("failed to list bucket objects.");
  return objects;
}

Directory::Directory(const std::string &path) : Object(path) {
  set_url(BuildUrl(path));
  set_type(S_IFDIR);
}

int Directory::Read(const Filler &filler) {
  return threads::Pool::Call(
      threads::PoolId::PR_REQ_0,
      [this, filler](base::Request *r) { return this->Read(r, filler); });
}

int Directory::Read(base::Request *req, const Filler &filler) {
  // for POSIX compliance
  filler(".");
  filler("..");

  std::string dir_path = path() + (path().empty() ? "" : "/");
  const size_t path_len = dir_path.size();

  ListReader reader(dir_path);
  std::list<std::string> keys, prefixes;
  int r;

  while ((r = reader.Read(req, &keys, &prefixes)) > 0) {
    for (const auto &prefix : prefixes) {
      // strip trailing slash
      std::string relative_path =
          prefix.substr(path_len, prefix.size() - path_len - 1);
      filler(relative_path);
      if (base::Config::precache_on_readdir()) {
        threads::Pool::CallAsync(
            threads::PoolId::PR_REQ_1,
            std::bind(Cache::Preload, std::placeholders::_1,
                      dir_path + relative_path, CacheHints::IS_DIR));
      }
    }

    for (const auto &key : keys) {
      if (dir_path == key) continue;
      std::string relative_path = key.substr(path_len);
      if (Object::IsInternalPath(relative_path)) {
        ++s_internal_objects_skipped_in_list;
        continue;
      }
      filler(relative_path);
      if (base::Config::precache_on_readdir()) {
        threads::Pool::CallAsync(
            threads::PoolId::PR_REQ_1,
            std::bind(Cache::Preload, std::placeholders::_1,
                      dir_path + relative_path, CacheHints::IS_FILE));
      }
    }
  }

  return r;
}

bool Directory::IsEmpty(base::Request *req) {
  // root directory isn't removable
  if (path().empty()) return false;

  // set max_keys to two because GET will always return the path we request
  ListReader reader(path() + "/", false, 2);
  std::list<std::string> keys;
  return (reader.Read(req, &keys, nullptr) == 1);
}

bool Directory::IsEmpty() {
  return threads::Pool::Call(
      threads::PoolId::PR_REQ_0,
      [this](base::Request *r) { return this->IsEmpty(r); });
}

int Directory::Remove(base::Request *req) {
  if (!IsEmpty(req)) return -ENOTEMPTY;
  return Object::Remove(req);
}

int Directory::Rename(base::Request *req, std::string to) {
  // can't do anything with the root directory
  if (path().empty()) return -EINVAL;

  to += "/";
  std::string from = path() + "/";
  const size_t from_len = from.size();

  Cache::Remove(path());
  ListReader reader(from, false);

  std::list<std::string> relative_paths;
  std::list<std::string> keys;
  int r;

  while ((r = reader.Read(req, &keys, nullptr)) > 0) {
    for (const auto &key : keys) {
      int rm_r = Cache::Remove(key);
      if (rm_r) return rm_r;
      relative_paths.push_back(key.substr(from_len));
    }
  }

  threads::ParallelWorkQueue<std::string> rename_queue(
      relative_paths.begin(), relative_paths.end(),
      std::bind(&CopyObject, std::placeholders::_1, std::placeholders::_2, from,
                to, false),
      std::bind(&CopyObject, std::placeholders::_1, std::placeholders::_2, from,
                to, true));
  r = rename_queue.Process();
  if (r) return r;

  threads::ParallelWorkQueue<std::string> delete_queue(
      relative_paths.begin(), relative_paths.end(),
      std::bind(&DeleteObject, std::placeholders::_1, std::placeholders::_2,
                from, false),
      std::bind(&DeleteObject, std::placeholders::_1, std::placeholders::_2,
                from, true));
  return delete_queue.Process();
}

}  // namespace fs
}  // namespace s3
