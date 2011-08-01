/*
 * object.cc
 * -------------------------------------------------------------------------
 * Read/write object metadata from/to S3.
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

#include <string.h>
#include <sys/xattr.h>

#include "config.h"
#include "directory.h"
#include "file.h"
#include "logger.h"
#include "object.h"
#include "request.h"
#include "service.h"
#include "symlink.h"

using namespace boost;
using namespace std;

using namespace s3;

namespace
{
  const int     BLOCK_SIZE                    = 512;

  const string  META_PREFIX_RESERVED          = "s3fuse-";

  const char   *META_PREFIX_RESERVED_CSTR     = META_PREFIX_RESERVED.c_str();
  const size_t  META_PREFIX_RESERVED_LEN      = META_PREFIX_RESERVED.size();
}

string object::build_url(const string &path, object_type type)
{
  if (type == OT_FILE)
    return file::build_url(path);
  else if (type == OT_DIRECTORY)
    return directory::build_url(path);
  else if (type == OT_SYMLINK)
    return symlink::build_url(path);
  else
    throw runtime_error("invalid object type.");
}

object::ptr object::create(const string &path, object_type type)
{
  if (type == OT_FILE)
    return ptr(new file(path));
  else if (type == OT_DIRECTORY)
    return ptr(new directory(path));
  else if (type == OT_SYMLINK)
    return ptr(new symlink(path));
  else
    throw runtime_error("invalid object type.");
}

object::object(const string &path)
  : _path(path),
    _expiry(0)
{
}

void object::init()
{
  memset(&_stat, 0, sizeof(_stat));

  _stat.st_nlink = 1; // laziness (see FUSE FAQ re. find)
  _stat.st_mode  = config::get_default_mode();
  _stat.st_uid   = config::get_default_uid();
  _stat.st_gid   = config::get_default_gid();

  if (_stat.st_uid == UID_MAX)
    _stat.st_uid = geteuid();

  if (_stat.st_gid == GID_MAX)
    _stat.st_gid = getegid();

  _stat.st_mode |= get_mode();
  _stat.st_mtime = time(NULL);

  _expiry = time(NULL) + config::get_cache_expiry_in_s();
  _url = object::build_url(_path, get_type());
  _content_type = config::get_default_content_type();
}

void object::copy_stat(struct stat *s)
{
  memcpy(s, &_stat, sizeof(_stat));
}

int object::set_metadata(const string &key, const string &value, int flags)
{
  mutex::scoped_lock lock(_mutex);
  meta_map::iterator itor = _metadata.find(key);

  if (strncmp(key.c_str(), META_PREFIX_RESERVED_CSTR, META_PREFIX_RESERVED_LEN) == 0)
    return -EINVAL;

  if (key == "__etag__" || key == "__content_type__")
    return -EINVAL;

  if (flags & XATTR_CREATE && itor != _metadata.end())
    return -EEXIST;

  if (flags & XATTR_REPLACE && itor == _metadata.end())
    return -ENODATA;

  _metadata[key] = value;
  return 0;
}

void object::get_metadata_keys(vector<string> *keys)
{
  mutex::scoped_lock lock(_mutex);

  keys->push_back("__etag__");
  keys->push_back("__content_type__");

  for (meta_map::const_iterator itor = _metadata.begin(); itor != _metadata.end(); ++itor)
    keys->push_back(itor->first);
}

int object::get_metadata(const string &key, string *value)
{
  mutex::scoped_lock lock(_mutex);
  meta_map::const_iterator itor;

  if (key == "__etag__")
    *value = _etag;
  else if (key == "__content_type__")
    *value = _content_type;
  else {
    itor = _metadata.find(key);

    if (itor == _metadata.end())
      return -ENODATA;

    *value = itor->second;
  }

  return 0;
}

int object::remove_metadata(const string &key)
{
  mutex::scoped_lock lock(_mutex);
  meta_map::iterator itor = _metadata.find(key);

  if (itor == _metadata.end())
    return -ENODATA;

  _metadata.erase(itor);
  return 0;
}

void object::set_mode(mode_t mode)
{
  mode = mode & ~S_IFMT;

  if (mode == 0)
    mode = config::get_default_mode();

  _stat.st_mode = (_stat.st_mode & S_IFMT) | mode;
}

void object::build_process_header(const request::ptr &req, const string &key, const string &value)
{
  // this doesn't need to lock the metadata mutex because the object won't be in the cache (and thus
  // isn't shareable) until the request has finished processing

  long long_value = strtol(value.c_str(), NULL, 0);
  const string &meta_prefix = service::get_meta_prefix();

  if (key == "Content-Type")
    _content_type = value;
  else if (key == "ETag")
    _etag = value;
  else if (key == "Content-Length")
    _stat.st_size = long_value;
  else if (key == (meta_prefix + "s3fuse-mode"))
    _stat.st_mode = long_value & ~S_IFMT;
  else if (key == (meta_prefix + "s3fuse-uid"))
    _stat.st_uid = long_value;
  else if (key == (meta_prefix + "s3fuse-gid"))
    _stat.st_gid = long_value;
  else if (key == (meta_prefix + "s3fuse-mtime"))
    _stat.st_mtime = long_value;
  else if (key == (meta_prefix + "s3fuse-mtime-etag"))
    _mtime_etag = value;
  else if (
    strncmp(key.c_str(), meta_prefix.c_str(), meta_prefix.size()) == 0 &&
    strncmp(key.c_str() + meta_prefix.size(), META_PREFIX_RESERVED_CSTR, META_PREFIX_RESERVED_LEN) != 0
  )
    _metadata[key.substr(meta_prefix.size())] = value;
}

void object::build_finalize(const request::ptr &req)
{
  time_t last_modified = req->get_last_modified();

  // this workaround is for cases when the file was updated by someone else and the mtime header wasn't set
  if (_mtime_etag != _etag && last_modified > _stat.st_mtime)
    _stat.st_mtime = last_modified;

  _mtime_etag = _etag;

  _stat.st_blocks = (_stat.st_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
  _expiry = time(NULL) + config::get_cache_expiry_in_s();
}

void object::set_meta_headers(const request::ptr &req)
{
  mutex::scoped_lock lock(_mutex);
  const string &meta_prefix = service::get_meta_prefix();
  char buf[16];

  // do this first so that we overwrite any keys we care about (i.e., those that start with "PREFIX-meta-s3fuse-")
  for (meta_map::const_iterator itor = _metadata.begin(); itor != _metadata.end(); ++itor)
    req->set_header(meta_prefix + itor->first, itor->second);

  snprintf(buf, 16, "%#o", _stat.st_mode & ~S_IFMT);
  req->set_header(meta_prefix + "s3fuse-mode", buf);

  snprintf(buf, 16, "%i", _stat.st_uid);
  req->set_header(meta_prefix + "s3fuse-uid", buf);

  snprintf(buf, 16, "%i", _stat.st_gid);
  req->set_header(meta_prefix + "s3fuse-gid", buf);

  snprintf(buf, 16, "%li", _stat.st_mtime);
  req->set_header(meta_prefix + "s3fuse-mtime", buf);

  req->set_header(meta_prefix + "s3fuse-mtime-etag", _mtime_etag);
  req->set_header("Content-Type", _content_type);
}

int object::commit_metadata(const request::ptr &req)
{
  req->init(HTTP_PUT);
  req->set_url(_url);
  req->set_header(service::get_header_prefix() + "copy-source", _url);
  req->set_header(service::get_header_prefix() + "copy-source-if-match", get_etag()); // get_etag() locks the metadata mutex
  req->set_header(service::get_header_prefix() + "metadata-directive", "REPLACE");

  set_meta_headers(req);

  req->run();

  if (req->get_response_code() != HTTP_SC_OK) {
    S3_LOG(LOG_WARNING, "object::commit_metadata", "failed to commit object metadata for [%s].\n", _url.c_str());
    return -EIO;
  }

  return 0;
}
