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

#include <string.h>
#include <sys/xattr.h>

#include <atomic>
#include <string>

#include "base/config.h"
#include "base/logger.h"
#include "base/request.h"
#include "base/statistics.h"
#include "base/timer.h"
#include "base/xml.h"
#include "fs/cache.h"
#include "fs/callback_xattr.h"
#include "fs/metadata.h"
#include "fs/object.h"
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
const int BLOCK_SIZE = 512;
const char *COMMIT_ETAG_XPATH = "/CopyObjectResult/ETag";
const char *VERSION_XPATH =
    "/ListVersionsResult/Version|/ListVersionsResult/DeleteMarker";

const std::string INTERNAL_OBJECT_PREFIX = "$s3fuse$_";
const char *INTERNAL_OBJECT_PREFIX_CSTR = INTERNAL_OBJECT_PREFIX.c_str();
const size_t INTERNAL_OBJECT_PREFIX_SIZE = INTERNAL_OBJECT_PREFIX.size();

// echo -n "" | md5sum
const char *EMPTY_VERSION_ETAG = "\"d41d8cd98f00b204e9800998ecf8427e\"";

#ifdef NEED_XATTR_PREFIX
const std::string XATTR_PREFIX = "user.";
const size_t XATTR_PREFIX_LEN = XATTR_PREFIX.size();
#else
const size_t XATTR_PREFIX_LEN = 0;
#endif

const std::string CONTENT_TYPE_XATTR = PACKAGE_NAME "_content_type";
const std::string ETAG_XATTR = PACKAGE_NAME "_etag";
const std::string CACHE_CONTROL_XATTR = PACKAGE_NAME "_cache_control";
const std::string CURRENT_VERSION_XATTR = PACKAGE_NAME "_current_version";
const std::string ALL_VERSIONS_XATTR = PACKAGE_NAME "_all_versions";
const std::string ALL_VERSIONS_INCL_EMPTY_XATTR =
    PACKAGE_NAME "_all_versions_incl_empty";

const int USER_XATTR_FLAGS =
    s3::fs::xattr::XM_WRITABLE | s3::fs::xattr::XM_SERIALIZABLE |
    s3::fs::xattr::XM_VISIBLE | s3::fs::xattr::XM_REMOVABLE |
    s3::fs::xattr::XM_COMMIT_REQUIRED;

const int META_XATTR_FLAGS =
    s3::fs::xattr::XM_WRITABLE | s3::fs::xattr::XM_VISIBLE |
    s3::fs::xattr::XM_REMOVABLE | s3::fs::xattr::XM_COMMIT_REQUIRED;

const char VERSION_SEPARATOR = '#';

std::atomic_int s_precon_failed_commits(0), s_new_etag_on_commit(0);
std::atomic_int s_commit_failures(0), s_precon_rescues(0),
    s_abandoned_commits(0);

void statistics_writer(std::ostream *o) {
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

inline std::string build_url_no_internal_check(const std::string &path) {
  return services::service::get_bucket_url() + "/" +
         base::request::url_encode(path);
}

inline std::string build_url_for_versioned_path(const std::string &path) {
  std::string::size_type sep = path.find(VERSION_SEPARATOR);
  if (sep == std::string::npos)
    throw std::runtime_error("can't build url for non-versioned path.");

  std::string base_path = path.substr(0, sep);
  std::string version = path.substr(sep + 1);

  return services::service::get_bucket_url() + "/" +
         base::request::url_encode(base_path) + "?versionId=" + version;
}

int set_nop_callback(const std::string & /* ignored */) { return 0; }

base::statistics::writers::entry s_writer(statistics_writer, 0);
} // namespace

int object::get_block_size() { return BLOCK_SIZE; }

bool object::is_internal_path(const std::string &path) {
  return strncmp(path.c_str(), INTERNAL_OBJECT_PREFIX_CSTR,
                 INTERNAL_OBJECT_PREFIX_SIZE) == 0;
}

bool object::is_versioned_path(const std::string &path) {
#ifdef WITH_AWS
  return base::config::get_enable_versioning() &&
         (path.find(VERSION_SEPARATOR) != std::string::npos);
#else
  return false;
#endif
}

std::string object::build_url(const std::string &path) {
  if (is_internal_path(path))
    throw std::runtime_error("path cannot start with " PACKAGE_NAME
                             " internal object prefix.");

  if (is_versioned_path(path))
    return build_url_for_versioned_path(path);

  return build_url_no_internal_check(path);
}

std::string object::build_internal_url(const std::string &key) {
  if (key.find('/') != std::string::npos)
    throw std::runtime_error("internal url key cannot contain a slash!");

  return build_url_no_internal_check(INTERNAL_OBJECT_PREFIX + key);
}

const std::string &object::get_internal_prefix() {
  return INTERNAL_OBJECT_PREFIX;
}

object::ptr object::create(const std::string &path,
                           const base::request::ptr &req) {
  ptr obj;

  if (!path.empty() && req->get_response_code() != base::HTTP_SC_OK)
    return obj;

  for (type_checker_list::const_iterator itor = type_checker_list::begin();
       itor != type_checker_list::end(); ++itor) {
    obj.reset(itor->second(path, req));

    if (obj)
      break;
  }

  if (!obj)
    throw std::runtime_error("couldn't figure out object type!");

  obj->init(req);

  return obj;
}

int object::copy_by_path(const base::request::ptr &req, const std::string &from,
                         const std::string &to) {
  req->init(base::HTTP_PUT);
  req->set_url(object::build_url(to));
  req->set_header(services::service::get_header_prefix() + "copy-source",
                  object::build_url(from));
  req->set_header(services::service::get_header_prefix() + "metadata-directive",
                  "COPY");

  // use transfer timeout because this could take a while
  req->run(base::config::get_transfer_timeout_in_s());

  return (req->get_response_code() == base::HTTP_SC_OK) ? 0 : -EIO;
}

int object::remove_by_url(const base::request::ptr &req,
                          const std::string &url) {
  req->init(base::HTTP_DELETE);
  req->set_url(url);

  req->run();

  return (req->get_response_code() == base::HTTP_SC_NO_CONTENT) ? 0 : -EIO;
}

object::object(const std::string &path) : _path(path), _expiry(0) {
  memset(&_stat, 0, sizeof(_stat));

  _stat.st_nlink = 1; // laziness (see FUSE FAQ re. find)
  _stat.st_blksize = BLOCK_SIZE;
  _stat.st_mode = base::config::get_default_mode() & ~S_IFMT;
  _stat.st_uid = base::config::get_default_uid();
  _stat.st_gid = base::config::get_default_gid();
  _stat.st_ctime = time(NULL);
  _stat.st_mtime = time(NULL);

  if (_stat.st_uid == UID_MAX)
    _stat.st_uid = getuid();

  if (_stat.st_gid == GID_MAX)
    _stat.st_gid = getgid();

  _content_type = base::config::get_default_content_type();

  if (!base::config::get_default_cache_control().empty())
    _metadata.replace(static_xattr::from_string(
        CACHE_CONTROL_XATTR, base::config::get_default_cache_control(),
        META_XATTR_FLAGS));

  _url = build_url(_path);
}

object::~object() {}

bool object::is_removable() { return true; }

void object::update_stat() {}

int object::set_metadata(const std::string &key, const char *value, size_t size,
                         int flags, bool *needs_commit) {
  std::lock_guard<std::mutex> lock(_mutex);
  std::string user_key = key.substr(XATTR_PREFIX_LEN);
  xattr_map::iterator itor = _metadata.find(user_key);

  *needs_commit = false;

#ifdef NEED_XATTR_PREFIX
  if (key.substr(0, XATTR_PREFIX_LEN) != XATTR_PREFIX)
    return -EINVAL;
#endif

  if (flags & XATTR_CREATE && itor != _metadata.end())
    return -EEXIST;

  if (itor == _metadata.end()) {
    if (flags & XATTR_REPLACE)
      return -ENOATTR;

    itor = _metadata.insert(static_xattr::create(user_key, USER_XATTR_FLAGS))
               .first;
  }

  // since we show read-only keys in get_metadata_keys(), an application might
  // reasonably assume that it can set them too.  since we don't want it failing
  // for no good reason, we'll fail silently.

  if (!itor->second->is_writable())
    return 0;

  *needs_commit = itor->second->is_commit_required();

  return itor->second->set_value(value, size);
}

void object::get_metadata_keys(std::vector<std::string> *keys) {
  std::lock_guard<std::mutex> lock(_mutex);

  for (xattr_map::const_iterator itor = _metadata.begin();
       itor != _metadata.end(); ++itor) {
    if (itor->second->is_visible()) {
#ifdef NEED_XATTR_PREFIX
      keys->push_back(XATTR_PREFIX + itor->first);
#else
      keys->push_back(itor->first);
#endif
    }
  }
}

int object::get_metadata(const std::string &key, char *buffer,
                         size_t max_size) {
  std::lock_guard<std::mutex> lock(_mutex);
  xattr::ptr value;
  std::string user_key = key.substr(XATTR_PREFIX_LEN);
  xattr_map::const_iterator itor;

#ifdef NEED_XATTR_PREFIX
  if (key.substr(0, XATTR_PREFIX_LEN) != XATTR_PREFIX)
    return -ENOATTR;
#endif

  itor = _metadata.find(user_key);

  if (itor == _metadata.end())
    return -ENOATTR;

  return itor->second->get_value(buffer, max_size);
}

int object::remove_metadata(const std::string &key) {
  std::lock_guard<std::mutex> lock(_mutex);
  xattr_map::iterator itor = _metadata.find(key.substr(XATTR_PREFIX_LEN));

  if (itor == _metadata.end() || !itor->second->is_removable())
    return -ENOATTR;

  _metadata.erase(itor);
  return 0;
}

void object::set_mode(mode_t mode) {
  mode = mode & ~S_IFMT;

  if (mode == 0)
    mode = base::config::get_default_mode() & ~S_IFMT;

  _stat.st_mode = (_stat.st_mode & S_IFMT) | mode;

  // successful chmod updates ctime
  _stat.st_ctime = time(NULL);
}

void object::init(const base::request::ptr &req) {
  // this doesn't need to lock the metadata mutex because the object won't be in
  // the cache (and thus isn't shareable) until the request has finished
  // processing

  const std::string &meta_prefix = services::service::get_header_meta_prefix();
  const std::string &cache_control = req->get_response_header("Cache-Control");
  mode_t mode;
  uid_t uid;
  gid_t gid;

  _content_type = req->get_response_header("Content-Type");
  _etag = req->get_response_header("ETag");

  _intact = (_etag == req->get_response_header(meta_prefix +
                                               metadata::LAST_UPDATE_ETAG));

  _stat.st_size =
      strtol(req->get_response_header("Content-Length").c_str(), NULL, 0);
  _stat.st_ctime = strtol(
      req->get_response_header(meta_prefix + metadata::CREATED_TIME).c_str(),
      NULL, 0);
  _stat.st_mtime = strtol(
      req->get_response_header(meta_prefix + metadata::LAST_MODIFIED_TIME)
          .c_str(),
      NULL, 0);

  mode = strtol(req->get_response_header(meta_prefix + metadata::MODE).c_str(),
                NULL, 0) &
         ~S_IFMT;
  uid = strtol(req->get_response_header(meta_prefix + metadata::UID).c_str(),
               NULL, 0);
  gid = strtol(req->get_response_header(meta_prefix + metadata::GID).c_str(),
               NULL, 0);

  for (base::header_map::const_iterator itor =
           req->get_response_headers().begin();
       itor != req->get_response_headers().end(); ++itor) {
    const std::string &key = itor->first;
    const std::string &value = itor->second;

    if (strncmp(key.c_str(), meta_prefix.c_str(), meta_prefix.size()) == 0 &&
        strncmp(key.c_str() + meta_prefix.size(), metadata::RESERVED_PREFIX,
                strlen(metadata::RESERVED_PREFIX)) != 0) {
      _metadata.replace(static_xattr::from_header(
          key.substr(meta_prefix.size()), value, USER_XATTR_FLAGS));
    }
  }

  _metadata.replace(static_xattr::from_string(CONTENT_TYPE_XATTR, _content_type,
                                              xattr::XM_VISIBLE));
  _metadata.replace(
      static_xattr::from_string(ETAG_XATTR, _etag, xattr::XM_VISIBLE));

  base::header_map::const_iterator version_itor =
      req->get_response_headers().find(services::service::get_header_prefix() +
                                       "version-id");
  if (version_itor == req->get_response_headers().end())
    _metadata.erase(CURRENT_VERSION_XATTR);
  else
    _metadata.replace(static_xattr::from_string(
        CURRENT_VERSION_XATTR, version_itor->second, xattr::XM_VISIBLE));

  if (cache_control.empty())
    _metadata.erase(CACHE_CONTROL_XATTR);
  else
    _metadata.replace(static_xattr::from_string(
        CACHE_CONTROL_XATTR, cache_control, META_XATTR_FLAGS));

  // this workaround is for cases when the file was updated by someone else and
  // the mtime header wasn't set
  if (!is_intact() && req->get_last_modified() > _stat.st_mtime)
    _stat.st_mtime = req->get_last_modified();

  // only accept uid, gid, mode from response if object is intact or if values
  // are non-zero (we do this so that objects created by some other mechanism
  // don't appear here with uid = 0, gid = 0, mode = 0)

  if (is_intact() || mode)
    _stat.st_mode = (_stat.st_mode & S_IFMT) | mode;

  if (is_intact() || uid)
    _stat.st_uid = uid;

  if (is_intact() || gid)
    _stat.st_gid = gid;

  _stat.st_blocks = (_stat.st_size + BLOCK_SIZE - 1) / BLOCK_SIZE;

  // setting _expiry > 0 makes this object valid
  _expiry = time(NULL) + base::config::get_cache_expiry_in_s();

#ifdef WITH_AWS
  if (base::config::get_allow_glacier_restores()) {
    _glacier = glacier::create(this, req);

    _metadata.replace(_glacier->get_storage_class_xattr());
    _metadata.replace(_glacier->get_restore_ongoing_xattr());
    _metadata.replace(_glacier->get_restore_expiry_xattr());
    _metadata.replace(_glacier->get_request_restore_xattr());
  }
#endif

#ifdef WITH_AWS
  if (base::config::get_enable_versioning()) {
    _metadata.replace(callback_xattr::create(
        ALL_VERSIONS_XATTR,
        std::bind(&object::get_all_versions, this, /* empties= */ false,
                  std::placeholders::_1),
        set_nop_callback, xattr::XM_VISIBLE));

    _metadata.replace(callback_xattr::create(
        ALL_VERSIONS_INCL_EMPTY_XATTR,
        std::bind(&object::get_all_versions, this, /* empties= */ true,
                  std::placeholders::_1),
        set_nop_callback, xattr::XM_VISIBLE));
  }
#endif
}

void object::set_request_headers(const base::request::ptr &req) {
  std::lock_guard<std::mutex> lock(_mutex);
  xattr_map::const_iterator itor;
  const std::string &meta_prefix = services::service::get_header_meta_prefix();
  char buf[16];

  // do this first so that we overwrite any keys we care about (i.e., those that
  // start with "META_PREFIX-META_PREFIX_RESERVED-")
  for (itor = _metadata.begin(); itor != _metadata.end(); ++itor) {
    std::string key, value;

    if (!itor->second->is_serializable())
      continue;

    itor->second->to_header(&key, &value);
    req->set_header(meta_prefix + key, value);
  }

  snprintf(buf, 16, "%#o", _stat.st_mode & ~S_IFMT);
  req->set_header(meta_prefix + metadata::MODE, buf);

  snprintf(buf, 16, "%i", _stat.st_uid);
  req->set_header(meta_prefix + metadata::UID, buf);

  snprintf(buf, 16, "%i", _stat.st_gid);
  req->set_header(meta_prefix + metadata::GID, buf);

  snprintf(buf, 16, "%li", _stat.st_ctime);
  req->set_header(meta_prefix + metadata::CREATED_TIME, buf);

  snprintf(buf, 16, "%li", _stat.st_mtime);
  req->set_header(meta_prefix + metadata::LAST_MODIFIED_TIME, buf);

  req->set_header(meta_prefix + metadata::LAST_UPDATE_ETAG, _etag);

  req->set_header("Content-Type", _content_type);

  itor = _metadata.find(CACHE_CONTROL_XATTR);

  if (itor != _metadata.end())
    req->set_header(
        "Cache-Control",
        std::dynamic_pointer_cast<static_xattr>(itor->second)->to_string());
}

void object::set_request_body(const base::request::ptr &req) {}

int object::commit(const base::request::ptr &req) {
  int current_error = 0, last_error = 0;

  // we may need to try to commit several times because:
  //
  // 1. the etag can change as a result of a copy
  // 2. we may get intermittent "precondition failed" errors

  for (int i = 0; i < base::config::get_max_inconsistent_state_retries(); i++) {
    base::xml::document_ptr doc;
    std::string response, new_etag;

    // save error from last iteration (so that we can tell if the precondition
    // failed retry worked)
    last_error = current_error;

    req->init(base::HTTP_PUT);
    req->set_url(_url);

    set_request_headers(req);

    // if the object already exists (i.e., if we have an etag) then just update
    // the metadata.

    if (_etag.empty()) {
      set_request_body(req);
    } else {
      req->set_header(services::service::get_header_prefix() + "copy-source",
                      _url);
      req->set_header(services::service::get_header_prefix() +
                          "copy-source-if-match",
                      _etag);
      req->set_header(services::service::get_header_prefix() +
                          "metadata-directive",
                      "REPLACE");
    }

    // this can, apparently, take a long time if the object is large
    req->run(base::config::get_transfer_timeout_in_s());

    if (req->get_response_code() == base::HTTP_SC_PRECONDITION_FAILED) {
      ++s_precon_failed_commits;
      S3_LOG(LOG_WARNING, "object::commit",
             "got precondition failed error for [%s].\n", _url.c_str());

      base::timer::sleep(i + 1);

      current_error = -EBUSY;
      continue;
    }

    if (req->get_response_code() != base::HTTP_SC_OK) {
      S3_LOG(LOG_WARNING, "object::commit",
             "failed to commit object metadata for [%s].\n", _url.c_str());

      current_error = -EIO;
      break;
    }

    response = req->get_output_string();

    // an empty response means the etag hasn't changed
    if (response.empty()) {
      current_error = 0;
      break;
    }

    // if we started out without an etag, then ignore anything we get here
    if (_etag.empty()) {
      current_error = 0;
      break;
    }

    doc = base::xml::parse(response);

    if (!doc) {
      S3_LOG(LOG_WARNING, "object::commit", "failed to parse response.\n");

      current_error = -EIO;
      break;
    }

    current_error = base::xml::find(doc, COMMIT_ETAG_XPATH, &new_etag);

    if (current_error)
      break;

    if (new_etag.empty()) {
      S3_LOG(LOG_WARNING, "object::commit", "no etag after commit.\n");

      current_error = -EIO;
      break;
    }

    // if the etag hasn't changed, don't re-commit
    if (new_etag == _etag) {
      current_error = 0;
      break;
    }

    ++s_new_etag_on_commit;
    S3_LOG(LOG_WARNING, "object::commit",
           "commit resulted in new etag. recommitting.\n");

    _etag = new_etag;
    current_error = -EAGAIN;
  }

  if (current_error) {
    if (current_error == -EIO) {
      ++s_commit_failures;
    } else {
      ++s_abandoned_commits;
      S3_LOG(LOG_WARNING, "object::commit", "giving up on [%s].\n",
             _url.c_str());
    }
  } else if (last_error == -EBUSY) {
    ++s_precon_rescues;
  }

  return current_error;
}

int object::remove(const base::request::ptr &req) {
  if (!is_removable())
    return -EBUSY;

  cache::remove(get_path());

  return object::remove_by_url(req, _url);
}

int object::rename(const base::request::ptr &req, const std::string &to) {
  int r;

  if (!is_removable())
    return -EBUSY;

  r = object::copy_by_path(req, _path, to);

  if (r)
    return r;

  cache::remove(get_path());

  return remove(req);
}

int object::fetch_all_versions(const base::request::ptr &req, bool empties,
                               std::string *out) {
  base::xml::document_ptr doc;

  req->init(base::HTTP_GET);
  req->set_url(services::service::get_bucket_url() + "?versions",
               std::string("prefix=") + base::request::url_encode(get_path()));
  req->run();

  if (req->get_response_code() != base::HTTP_SC_OK)
    return -EIO;

  doc = base::xml::parse(req->get_output_string());

  if (!doc) {
    S3_LOG(LOG_WARNING, "object::fetch_all_versions",
           "failed to parse response.\n");
    return -EIO;
  }

  base::xml::element_map_list versions;
  base::xml::find(doc, VERSION_XPATH, &versions);
  std::string versions_str;
  std::string latest_etag;
  int empty_count = 0;

  for (base::xml::element_map_list::iterator itor = versions.begin();
       itor != versions.end(); ++itor) {
    base::xml::element_map &keys = *itor;

    const std::string &key = keys["Key"];
    if (key != get_path())
      continue;

    const std::string &etag = keys["ETag"];
    if (etag == latest_etag)
      continue;
    if (!empties && etag == EMPTY_VERSION_ETAG) {
      empty_count++;
      continue;
    }
    latest_etag = etag;

    if (keys[base::xml::MAP_NAME_KEY] == "Version") {
      versions_str += "version=" + keys["VersionId"] +
                      " mtime=" + keys["LastModified"] + " etag=" + etag +
                      " size=" + keys["Size"] + "\n";
    } else if (keys[base::xml::MAP_NAME_KEY] == "DeleteMarker") {
      versions_str += "version=" + keys["VersionId"] +
                      " mtime=" + keys["LastModified"] + " etag=" + etag +
                      " deleted\n";
    }
  }

  if (empty_count) {
    if (!versions_str.empty())
      versions_str += "\n";
    versions_str += "(" + std::to_string(empty_count) +
                    " empty version(s) omitted. Request extended attribute \"" +
                    ALL_VERSIONS_INCL_EMPTY_XATTR +
                    "\" to see empty versions.)\n";
  }

  *out = versions_str;
  return 0;
}

int object::get_all_versions(bool empties, std::string *out) {
  return threads::pool::call(threads::PR_REQ_1,
                             std::bind(&object::fetch_all_versions, this,
                                       std::placeholders::_1, empties, out));
}

} // namespace fs
} // namespace s3
