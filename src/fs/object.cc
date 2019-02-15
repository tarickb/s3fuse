/*
 * fs/object.cc
 * -------------------------------------------------------------------------
 * Read/write object metadata from/to S3.
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

#include "fs/object.h"

#include <string.h>
#include <sys/xattr.h>

#include <atomic>
#include <list>
#include <map>
#include <string>

#include "base/config.h"
#include "base/logger.h"
#include "base/request.h"
#include "base/statistics.h"
#include "base/timer.h"
#include "base/url.h"
#include "base/xml.h"
#include "fs/cache.h"
#include "fs/callback_xattr.h"
#include "fs/metadata.h"
#include "fs/static_xattr.h"
#include "services/service.h"

#ifdef WITH_AWS
#include "fs/glacier.h"
#endif

#ifndef ENOATTR
#ifndef ENODATA
#error Need either ENOATTR or ENODATA!
#endif

#define ENOATTR ENODATA
#endif

#ifndef __APPLE__
#define NEED_XATTR_PREFIX
#endif

namespace s3 {
namespace fs {

namespace {
constexpr int BLOCK_SIZE = 512;
constexpr char INTERNAL_OBJECT_PREFIX[] = "$s3fuse$_";
constexpr char COMMIT_ETAG_XPATH[] = "/CopyObjectResult/ETag";

#ifdef NEED_XATTR_PREFIX
const std::string XATTR_PREFIX = "user.";
const size_t XATTR_PREFIX_LEN = XATTR_PREFIX.size();
#else
const size_t XATTR_PREFIX_LEN = 0;
#endif

constexpr char CONTENT_TYPE_XATTR[] = PACKAGE_NAME "_content_type";
constexpr char ETAG_XATTR[] = PACKAGE_NAME "_etag";
constexpr char CACHE_CONTROL_XATTR[] = PACKAGE_NAME "_cache_control";

constexpr char CURRENT_VERSION_XATTR[] = PACKAGE_NAME "_current_version";
constexpr char ALL_VERSIONS_XATTR[] = PACKAGE_NAME "_all_versions";
constexpr char ALL_VERSIONS_INCL_EMPTY_XATTR[] =
    PACKAGE_NAME "_all_versions_incl_empty";

constexpr int USER_XATTR_FLAGS =
    s3::fs::XAttr::XM_WRITABLE | s3::fs::XAttr::XM_SERIALIZABLE |
    s3::fs::XAttr::XM_VISIBLE | s3::fs::XAttr::XM_REMOVABLE |
    s3::fs::XAttr::XM_COMMIT_REQUIRED;

constexpr int META_XATTR_FLAGS =
    s3::fs::XAttr::XM_WRITABLE | s3::fs::XAttr::XM_VISIBLE |
    s3::fs::XAttr::XM_REMOVABLE | s3::fs::XAttr::XM_COMMIT_REQUIRED;

constexpr char VERSION_SEPARATOR = '#';

std::atomic_int s_precon_failed_commits(0), s_new_etag_on_commit(0);
std::atomic_int s_commit_failures(0), s_precon_rescues(0),
    s_abandoned_commits(0);

void StatsWriter(std::ostream *o) {
  *o << "objects:\n"
        "  precondition failed during commit: "
     << s_precon_failed_commits
     << "\n"
        "  new etag on commit: "
     << s_new_etag_on_commit
     << "\n"
        "  commit failures: "
     << s_commit_failures
     << "\n"
        "  precondition failed rescues: "
     << s_precon_rescues
     << "\n"
        "  abandoned commits: "
     << s_abandoned_commits << "\n";
}

inline std::string BuildUrlNoInternalCheck(const std::string &path) {
  return services::Service::bucket_url() + "/" + base::Url::Encode(path);
}

inline std::string BuildUrlForVersionedPath(const std::string &path) {
  auto sep = path.find(VERSION_SEPARATOR);
  if (sep == std::string::npos)
    throw std::runtime_error("can't build url for non-versioned path.");
  std::string base_path = path.substr(0, sep);
  std::string version = path.substr(sep + 1);
  return services::Service::versioning()->BuildVersionedUrl(base_path, version);
}

base::Statistics::Writers::Entry s_writer(StatsWriter, 0);
}  // namespace

int Object::block_size() { return BLOCK_SIZE; }

std::string Object::internal_prefix() { return INTERNAL_OBJECT_PREFIX; }

std::string Object::BuildUrl(const std::string &path) {
  if (IsInternalPath(path))
    throw std::runtime_error("path cannot start with " PACKAGE_NAME
                             " internal Object:: prefix.");
  if (IsVersionedPath(path)) return BuildUrlForVersionedPath(path);
  return BuildUrlNoInternalCheck(path);
}

std::string Object::BuildInternalUrl(const std::string &key) {
  if (key.find('/') != std::string::npos)
    throw std::runtime_error("internal url key cannot contain a slash!");
  return BuildUrlNoInternalCheck(INTERNAL_OBJECT_PREFIX + key);
}

bool Object::IsInternalPath(const std::string &path) {
  return strncmp(path.c_str(), INTERNAL_OBJECT_PREFIX,
                 sizeof(INTERNAL_OBJECT_PREFIX) - 1) == 0;
}

bool Object::IsVersionedPath(const std::string &path) {
  return base::Config::enable_versioning() &&
         services::Service::versioning() != nullptr &&
         (path.find(VERSION_SEPARATOR) != std::string::npos);
}

int Object::CopyByPath(base::Request *req, const std::string &from,
                       const std::string &to) {
  req->Init(base::HttpMethod::PUT);
  req->SetUrl(Object::BuildUrl(to));
  req->SetHeader(services::Service::header_prefix() + "copy-source",
                 Object::BuildUrl(from));
  req->SetHeader(services::Service::header_prefix() + "metadata-directive",
                 "COPY");
  // use transfer timeout because this could take a while
  req->Run(base::Config::transfer_timeout_in_s());
  return (req->response_code() == base::HTTP_SC_OK) ? 0 : -EIO;
}

int Object::RemoveByUrl(base::Request *req, const std::string &url) {
  req->Init(base::HttpMethod::DELETE);
  req->SetUrl(url);
  req->Run();
  return (req->response_code() == base::HTTP_SC_NO_CONTENT) ? 0 : -EIO;
}

std::shared_ptr<Object> Object::Create(const std::string &path,
                                       base::Request *req) {
  if (!path.empty() && req->response_code() != base::HTTP_SC_OK) return {};
  std::shared_ptr<Object> obj;
  for (auto iter = TypeCheckers::begin(); iter != TypeCheckers::end(); ++iter) {
    obj.reset(iter->second(path, req));
    if (obj) break;
  }
  if (!obj) throw std::runtime_error("couldn't figure out object type!");
  obj->Init(req);
  return obj;
}

Object::~Object() {}

bool Object::IsRemovable() { return true; }

int Object::Remove(base::Request *req) {
  if (!IsRemovable()) return -EBUSY;
  Cache::Remove(path_);
  return Object::RemoveByUrl(req, url_);
}

int Object::Rename(base::Request *req, std::string to) {
  if (!IsRemovable()) return -EBUSY;
  int r = Object::CopyByPath(req, path_, to);
  if (r) return r;
  Cache::Remove(path_);
  return Remove(req);
}

std::vector<std::string> Object::GetMetadataKeys() {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<std::string> keys;
  for (const auto &attr : metadata_) {
    if (attr.second->is_visible()) {
#ifdef NEED_XATTR_PREFIX
      keys.push_back(XATTR_PREFIX + attr.first);
#else
      keys.push_back(attr.first);
#endif
    }
  }
  return keys;
}

int Object::GetMetadata(const std::string &key, char *buffer, size_t max_size) {
#ifdef NEED_XATTR_PREFIX
  if (key.substr(0, XATTR_PREFIX_LEN) != XATTR_PREFIX) return -ENOATTR;
#endif
  std::lock_guard<std::mutex> lock(mutex_);
  auto iter = metadata_.find(key.substr(XATTR_PREFIX_LEN));
  if (iter == metadata_.end()) return -ENOATTR;
  return iter->second->GetValue(buffer, max_size);
}

int Object::SetMetadata(const std::string &key, const char *value, size_t size,
                        int flags, bool *needs_commit) {
#ifdef NEED_XATTR_PREFIX
  if (key.substr(0, XATTR_PREFIX_LEN) != XATTR_PREFIX) return -EINVAL;
#endif
  const std::string user_key = key.substr(XATTR_PREFIX_LEN);
  std::lock_guard<std::mutex> lock(mutex_);
  auto iter = metadata_.find(user_key);
  *needs_commit = false;
  if (flags & XATTR_CREATE && iter != metadata_.end()) return -EEXIST;
  if (iter == metadata_.end()) {
    if (flags & XATTR_REPLACE) return -ENOATTR;
    iter = UpdateMetadata(StaticXAttr::Create(user_key, USER_XATTR_FLAGS));
  }
  // since we show read-only keys in GetMetadataKeys(), an application might
  // reasonably assume that it can set them too.  since we don't want it failing
  // for no good reason, we'll fail silently.
  if (!iter->second->is_writable()) return 0;
  *needs_commit = iter->second->is_commit_required();
  return iter->second->SetValue(value, size);
}

int Object::RemoveMetadata(const std::string &key) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto iter = metadata_.find(key.substr(XATTR_PREFIX_LEN));
  if (iter == metadata_.end() || !iter->second->is_removable()) return -ENOATTR;
  metadata_.erase(iter);
  return 0;
}

void Object::SetMode(mode_t mode) {
  mode = mode & ~S_IFMT;
  if (mode == 0) mode = base::Config::default_mode() & ~S_IFMT;
  stat_.st_mode = (stat_.st_mode & S_IFMT) | mode;
  // successful chmod updates ctime
  stat_.st_ctime = time(nullptr);
}

int Object::Commit(base::Request *req) {
  int current_error = 0, last_error = 0;

  // we may need to try to commit several times because:
  //
  // 1. the etag can change as a result of a copy
  // 2. we may get intermittent "precondition failed" errors

  for (int i = 0; i < base::Config::max_inconsistent_state_retries(); i++) {
    // save error from last iteration (so that we can tell if the precondition
    // failed retry worked)
    last_error = current_error;

    req->Init(base::HttpMethod::PUT);
    req->SetUrl(url_);
    SetRequestHeaders(req);

    // if the object already exists (i.e., if we have an etag) then just update
    // the metadata.
    if (etag_.empty()) {
      SetRequestBody(req);
    } else {
      req->SetHeader(services::Service::header_prefix() + "copy-source", url_);
      req->SetHeader(
          services::Service::header_prefix() + "copy-source-if-match", etag_);
      req->SetHeader(services::Service::header_prefix() + "metadata-directive",
                     "REPLACE");
    }

    // this can, apparently, take a long time if the object is large
    req->Run(base::Config::transfer_timeout_in_s());

    if (req->response_code() == base::HTTP_SC_PRECONDITION_FAILED) {
      ++s_precon_failed_commits;
      S3_LOG(LOG_WARNING, "Object::Commit",
             "got precondition failed error for [%s].\n", url_.c_str());
      base::Timer::Sleep(i + 1);
      current_error = -EBUSY;
      continue;
    }

    if (req->response_code() != base::HTTP_SC_OK) {
      S3_LOG(LOG_WARNING, "Object::Commit",
             "failed to commit Object:: metadata for [%s].\n", url_.c_str());
      current_error = -EIO;
      break;
    }

    const std::string response = req->GetOutputAsString();
    // an empty response means the etag hasn't changed
    if (response.empty()) {
      current_error = 0;
      break;
    }
    // if we started out without an etag, then ignore anything we get here
    if (etag_.empty()) {
      current_error = 0;
      break;
    }

    auto doc = base::XmlDocument::Parse(response);
    if (!doc) {
      S3_LOG(LOG_WARNING, "Object::Commit", "failed to parse response.\n");
      current_error = -EIO;
      break;
    }

    std::string new_etag;
    current_error = doc->Find(COMMIT_ETAG_XPATH, &new_etag);
    if (current_error) break;
    if (new_etag.empty()) {
      S3_LOG(LOG_WARNING, "Object::Commit", "no etag after commit.\n");
      current_error = -EIO;
      break;
    }

    // if the etag hasn't changed, don't re-commit
    if (new_etag == etag_) {
      current_error = 0;
      break;
    }

    ++s_new_etag_on_commit;
    S3_LOG(LOG_WARNING, "Object::Commit",
           "commit resulted in new etag. recommitting.\n");
    etag_ = new_etag;
    current_error = -EAGAIN;
  }

  if (current_error) {
    if (current_error == -EIO) {
      ++s_commit_failures;
    } else {
      ++s_abandoned_commits;
      S3_LOG(LOG_WARNING, "Object::Commit", "giving up on [%s].\n",
             url_.c_str());
    }
  } else if (last_error == -EBUSY) {
    ++s_precon_rescues;
  }

  return current_error;
}

int Object::Commit() {
  return threads::Pool::Call(threads::PoolId::PR_REQ_0,
                             [this](base::Request *r) { return Commit(r); });
}

int Object::Remove() {
  return threads::Pool::Call(threads::PoolId::PR_REQ_0,
                             [this](base::Request *r) { return Remove(r); });
}

int Object::Rename(std::string to) {
  return threads::Pool::Call(
      threads::PoolId::PR_REQ_0,
      [this, to](base::Request *r) { return Rename(r, to); });
}

Object::Object(const std::string &path) : path_(path), expiry_(0) {
  memset(&stat_, 0, sizeof(stat_));
  stat_.st_nlink = 1;  // laziness (see FUSE FAQ re. find)
  stat_.st_blksize = BLOCK_SIZE;
  stat_.st_mode = base::Config::default_mode() & ~S_IFMT;
  stat_.st_uid = base::Config::default_uid();
  stat_.st_gid = base::Config::default_gid();
  stat_.st_ctime = time(nullptr);
  stat_.st_mtime = time(nullptr);
  if (stat_.st_uid == UID_MAX) stat_.st_uid = getuid();
  if (stat_.st_gid == GID_MAX) stat_.st_gid = getgid();
  if (!base::Config::default_cache_control().empty())
    UpdateMetadata(StaticXAttr::FromString(
        CACHE_CONTROL_XATTR, base::Config::default_cache_control(),
        META_XATTR_FLAGS));
  content_type_ = base::Config::default_content_type();
  url_ = BuildUrl(path_);
}

void Object::Init(base::Request *req) {
  // this doesn't need to lock the metadata mutex because the Object:: won't be
  // in the cache (and thus isn't shareable) until the request has finished
  // processing

  const std::string meta_prefix = services::Service::header_meta_prefix();

  content_type_ = req->response_header("Content-Type");
  etag_ = req->response_header("ETag");
  intact_ =
      (etag_ == req->response_header(meta_prefix + Metadata::LAST_UPDATE_ETAG));
  stat_.st_size =
      strtol(req->response_header("Content-Length").c_str(), nullptr, 0);
  stat_.st_ctime =
      strtol(req->response_header(meta_prefix + Metadata::CREATED_TIME).c_str(),
             nullptr, 0);
  stat_.st_mtime = strtol(
      req->response_header(meta_prefix + Metadata::LAST_MODIFIED_TIME).c_str(),
      nullptr, 0);

  mode_t mode =
      strtol(req->response_header(meta_prefix + Metadata::MODE).c_str(),
             nullptr, 0) &
      ~S_IFMT;
  uid_t uid = strtol(req->response_header(meta_prefix + Metadata::UID).c_str(),
                     nullptr, 0);
  gid_t gid = strtol(req->response_header(meta_prefix + Metadata::GID).c_str(),
                     nullptr, 0);

  for (const auto &header : req->response_headers()) {
    const std::string &key = header.first;
    const std::string &value = header.second;

    if (strncmp(key.c_str(), meta_prefix.c_str(), meta_prefix.size()) == 0 &&
        strncmp(key.c_str() + meta_prefix.size(), Metadata::RESERVED_PREFIX,
                strlen(Metadata::RESERVED_PREFIX)) != 0) {
      auto xattr = StaticXAttr::FromHeader(key.substr(meta_prefix.size()),
                                           value, USER_XATTR_FLAGS);
      UpdateMetadata(std::move(xattr));
    }
  }

  UpdateMetadata(StaticXAttr::FromString(CONTENT_TYPE_XATTR, content_type_,
                                         XAttr::XM_VISIBLE));
  UpdateMetadata(StaticXAttr::FromString(ETAG_XATTR, etag_, XAttr::XM_VISIBLE));

  if (services::Service::versioning()) {
    const std::string version =
        services::Service::versioning()->ExtractCurrentVersion(req);
    if (version.empty()) {
      metadata_.erase(CURRENT_VERSION_XATTR);
    } else {
      UpdateMetadata(StaticXAttr::FromString(CURRENT_VERSION_XATTR, version,
                                             XAttr::XM_VISIBLE));
    }
  }

  const std::string cache_control = req->response_header("Cache-Control");
  if (cache_control.empty())
    metadata_.erase(CACHE_CONTROL_XATTR);
  else
    UpdateMetadata(StaticXAttr::FromString(CACHE_CONTROL_XATTR, cache_control,
                                           META_XATTR_FLAGS));

  // this workaround is for cases when the file was updated by someone else and
  // the mtime header wasn't set
  if (!intact() && req->last_modified() > stat_.st_mtime)
    stat_.st_mtime = req->last_modified();

  // only accept uid, gid, mode from response if object is intact or if values
  // are non-zero (we do this so that objects created by some other mechanism
  // don't appear here with uid = 0, gid = 0, mode = 0)
  if (intact() || mode) stat_.st_mode = (stat_.st_mode & S_IFMT) | mode;
  if (intact() || uid) stat_.st_uid = uid;
  if (intact() || gid) stat_.st_gid = gid;

  stat_.st_blocks = (stat_.st_size + BLOCK_SIZE - 1) / BLOCK_SIZE;

  // setting expiry_ > 0 makes this Object:: valid
  expiry_ = time(nullptr) + base::Config::cache_expiry_in_s();

#ifdef WITH_AWS
  if (base::Config::allow_glacier_restores()) {
    glacier_ = Glacier::Create(path_, url_, req);
    for (auto &attr : glacier_->BuildXAttrs()) UpdateMetadata(std::move(attr));
  }
#endif

  if (base::Config::enable_versioning() &&
      services::Service::versioning() != nullptr) {
    UpdateMetadata(CallbackXAttr::Create(
        ALL_VERSIONS_XATTR,
        [this](std::string *out) {
          return threads::Pool::Call(
              threads::PoolId::PR_REQ_1,
              std::bind(&Object::FetchAllVersions, this,
                        services::VersionFetchOptions::NONE,
                        std::placeholders::_1, out));
        },
        [](std::string) { return 0; }, XAttr::XM_VISIBLE));

    UpdateMetadata(CallbackXAttr::Create(
        ALL_VERSIONS_INCL_EMPTY_XATTR,
        [this](std::string *out) {
          return threads::Pool::Call(
              threads::PoolId::PR_REQ_1,
              std::bind(&Object::FetchAllVersions, this,
                        services::VersionFetchOptions::WITH_EMPTIES,
                        std::placeholders::_1, out));
        },
        [](std::string) { return 0; }, XAttr::XM_VISIBLE));
  }
}

void Object::SetRequestHeaders(base::Request *req) {
  std::lock_guard<std::mutex> lock(mutex_);
  const std::string meta_prefix = services::Service::header_meta_prefix();

  // do this first so that we overwrite any keys we care about (i.e., those that
  // start with "META_PREFIX-META_PREFIX_RESERVED-")
  for (const auto &meta : metadata_) {
    std::string key, value;
    if (!meta.second->is_serializable()) continue;
    meta.second->ToHeader(&key, &value);
    req->SetHeader(meta_prefix + key, value);
  }

  char buf[16];
  snprintf(buf, sizeof(buf), "%#o", stat_.st_mode & ~S_IFMT);
  req->SetHeader(meta_prefix + Metadata::MODE, buf);
  req->SetHeader(meta_prefix + Metadata::UID, std::to_string(stat_.st_uid));
  req->SetHeader(meta_prefix + Metadata::GID, std::to_string(stat_.st_gid));
  req->SetHeader(meta_prefix + Metadata::CREATED_TIME,
                 std::to_string(stat_.st_ctime));
  req->SetHeader(meta_prefix + Metadata::LAST_MODIFIED_TIME,
                 std::to_string(stat_.st_mtime));

  req->SetHeader(meta_prefix + Metadata::LAST_UPDATE_ETAG, etag_);
  req->SetHeader("Content-Type", content_type_);

  auto iter = metadata_.find(CACHE_CONTROL_XATTR);
  if (iter != metadata_.end())
    req->SetHeader("Cache-Control", iter->second->ToString());
}

void Object::SetRequestBody(base::Request *req) {}

void Object::UpdateStat() {}

Object::MetadataMap::iterator Object::UpdateMetadata(
    std::unique_ptr<XAttr> attr) {
  return metadata_.insert(std::make_pair(attr->key(), std::move(attr))).first;
}

int Object::FetchAllVersions(services::VersionFetchOptions options,
                             base::Request *req, std::string *out) {
  int empty_count = 0;

  int r = services::Service::versioning()->FetchAllVersions(options, path_, req,
                                                            out, &empty_count);
  if (r) return r;

  if (empty_count) {
    if (!out->empty()) (*out) += "\n";
    *out += "(" + std::to_string(empty_count) +
            " empty version(s) omitted. Request extended attribute \"" +
            ALL_VERSIONS_INCL_EMPTY_XATTR + "\" to see empty versions.)\n";
  }

  return 0;
}

}  // namespace fs
}  // namespace s3
