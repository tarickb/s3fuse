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

#include <boost/lexical_cast.hpp>

#include "config.h"
#include "directory.h"
#include "encrypted_file.h"
#include "file.h"
#include "logger.h"
#include "object.h"
#include "request.h"
#include "service.h"
#include "symlink.h"
#include "xattr.h"

#ifndef ENOATTR
  #ifndef ENODATA
    #error Need either ENOATTR or ENODATA!
  #endif

  #define ENOATTR ENODATA
#endif

using namespace boost;
using namespace std;

using namespace s3;

namespace
{
  const int     BLOCK_SIZE                    = 512;
  const string  META_PREFIX                   = "meta-";
  const string  META_PREFIX_RESERVED          = "s3fuse-";

  const char   *META_PREFIX_RESERVED_CSTR     = META_PREFIX_RESERVED.c_str();
  const size_t  META_PREFIX_RESERVED_LEN      = META_PREFIX_RESERVED.size();
}

string object::build_url(const string &path)
{
  return service::get_bucket_url() + "/" + util::url_encode(path);
}

object::ptr object::create(const string &path, const request::ptr &req)
{
  const string &content_type = req->get_response_header("Content-Type");
  object *o = NULL;

  if (req->get_response_code() == HTTP_SC_OK) {
    if (directory::is_valid_url(req->get_url()))
      o = new directory(path);
    else if (content_type == symlink::get_default_content_type())
      o = new symlink(path);
    else if (content_type == encrypted_file::get_default_content_type())
      o = new encrypted_file(path);
    else
      o = new file(path);

    o->init(req);
  }

  return object::ptr(o);
}

object::object(const string &path)
  : _path(path),
    _expiry(0)
{
  memset(&_stat, 0, sizeof(_stat));

  _stat.st_nlink = 1; // laziness (see FUSE FAQ re. find)
  _stat.st_blksize = BLOCK_SIZE;
  _stat.st_mode = config::get_default_mode();
  _stat.st_uid = config::get_default_uid();
  _stat.st_gid = config::get_default_gid();
  _stat.st_mtime = time(NULL);

  if (_stat.st_uid == UID_MAX)
    _stat.st_uid = geteuid();

  if (_stat.st_gid == GID_MAX)
    _stat.st_gid = getegid();

  _content_type = config::get_default_content_type();
  _url = build_url(_path);
}

object::~object()
{
}

int object::set_metadata(const string &key, const char *value, size_t size, int flags)
{
  mutex::scoped_lock lock(_metadata_mutex);
  string user_key = key.substr(config::get_xattr_prefix().size());
  xattr_map::iterator itor = _metadata.find(user_key);

  if (key.substr(0, config::get_xattr_prefix().size()) != config::get_xattr_prefix())
    return -EINVAL;

  if (strncmp(user_key.c_str(), META_PREFIX_RESERVED_CSTR, META_PREFIX_RESERVED_LEN) == 0)
    return -EINVAL;

  // since we show these keys in get_metadata_keys(), an application might reasonably assume
  // that it can set them too.  since we don't want it failing for no good reason, we'll
  // fail silently.
  if (user_key == "__md5__" || user_key == "__etag__" || user_key == "__content_type__")
    return 0;

  if (flags & XATTR_CREATE && itor != _metadata.end())
    return -EEXIST;

  if (itor == _metadata.end()) {
    if (flags & XATTR_REPLACE)
      return -ENOATTR;

    itor = _metadata.insert(make_pair(user_key, xattr::create(user_key))).first;
  }

  itor->second->set_value(value, size);

  return 0;
}

void object::get_metadata_keys(vector<string> *keys)
{
  mutex::scoped_lock lock(_metadata_mutex);

  keys->push_back(config::get_xattr_prefix() + "__md5__");
  keys->push_back(config::get_xattr_prefix() + "__etag__");
  keys->push_back(config::get_xattr_prefix() + "__content_type__");

  for (xattr_map::const_iterator itor = _metadata.begin(); itor != _metadata.end(); ++itor)
    keys->push_back(config::get_xattr_prefix() + itor->first);
}

int object::get_metadata(const string &key, char *buffer, size_t max_size)
{
  mutex::scoped_lock lock(_metadata_mutex);
  xattr::ptr value;
  string user_key = key.substr(config::get_xattr_prefix().size());

  if (key.substr(0, config::get_xattr_prefix().size()) != config::get_xattr_prefix())
    return -ENOATTR;

  if (user_key == "__md5__")
    value = xattr::from_string(user_key, _md5);
  else if (user_key == "__etag__")
    value = xattr::from_string(user_key, _etag);
  else if (user_key == "__content_type__")
    value = xattr::from_string(user_key, _content_type);
  else {
    xattr_map::const_iterator itor = _metadata.find(user_key);

    if (itor != _metadata.end())
      value = itor->second;
  }

  return value ? value->get_value(buffer, max_size) : -ENOATTR;
}

int object::remove_metadata(const string &key)
{
  mutex::scoped_lock lock(_metadata_mutex);
  xattr_map::iterator itor = _metadata.find(key.substr(config::get_xattr_prefix().size()));

  if (itor == _metadata.end())
    return -ENOATTR;

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

void object::init(const request::ptr &req)
{
  // this doesn't need to lock the metadata mutex because the object won't be in the cache (and thus
  // isn't shareable) until the request has finished processing

  string meta_prefix = service::get_header_prefix() + META_PREFIX;

  _content_type = req->get_response_header("Content-Type");
  _etag = req->get_response_header("ETag");
  _stat.st_size = lexical_cast<size_t>(req->get_response_header("Content-Length"));
  _stat.st_mode = lexical_cast<mode_t>(req->get_response_header(meta_prefix + "s3fuse-mode")) & ~S_IFMT;
  _stat.st_uid = lexical_cast<uid_t>(req->get_response_header(meta_prefix + "s3fuse-uid"));
  _stat.st_gid = lexical_cast<gid_t>(req->get_response_header(meta_prefix + "s3fuse-gid"));
  _stat.st_mtime = lexical_cast<time_t>(req->get_response_header(meta_prefix + "s3fuse-mtime"));
  _mtime_etag = req->get_response_header(meta_prefix + "s3fuse-mtime-etag");
  _md5 = req->get_response_header(meta_prefix + "s3fuse-md5");
  _md5_etag = req->get_response_header(meta_prefix + "s3fuse-md5-etag");

  for (header_map::const_iterator itor = req->get_response_headers().begin(); itor != req->get_response_headers().end(); ++itor) {
    const string &key = itor->first;
    const string &value = itor->second;

    if (
      strncmp(key.c_str(), meta_prefix.c_str(), meta_prefix.size()) == 0 &&
      strncmp(key.c_str() + meta_prefix.size(), META_PREFIX_RESERVED_CSTR, META_PREFIX_RESERVED_LEN) != 0
    ) {
      xattr::ptr attr = xattr::from_header(key.substr(meta_prefix.size()), value);

      _metadata[attr->get_key()] = attr;
    }
  }

  // this workaround is for cases when the file was updated by someone else and the mtime header wasn't set
  if (_mtime_etag != _etag && req->get_last_modified() > _stat.st_mtime)
    _stat.st_mtime = req->get_last_modified();

  _mtime_etag = _etag;

  // this workaround is for multipart uploads, which don't get a valid md5 etag
  if (!util::is_valid_md5(_md5))
    _md5.clear();

  if ((_md5_etag != _etag || _md5.empty()) && util::is_valid_md5(_etag))
    _md5 = _etag;

  _md5_etag = _etag;

  _stat.st_blocks = (_stat.st_size + BLOCK_SIZE - 1) / BLOCK_SIZE;

  // setting _expiry > 0 makes this object valid
  _expiry = time(NULL) + config::get_cache_expiry_in_s();
}

void object::request_set_meta_headers(request *req)
{
  mutex::scoped_lock lock(_metadata_mutex);
  string meta_prefix = service::get_header_prefix() + META_PREFIX;
  char buf[16];

  // do this first so that we overwrite any keys we care about (i.e., those that start with "PREFIX-meta-s3fuse-")
  for (xattr_map::const_iterator itor = _metadata.begin(); itor != _metadata.end(); ++itor) {
    string key, value;

    itor->second->to_header(&key, &value);
    req->set_header(meta_prefix + key, value);
  }

  snprintf(buf, 16, "%#o", _stat.st_mode & ~S_IFMT);
  req->set_header(meta_prefix + "s3fuse-mode", buf);

  snprintf(buf, 16, "%i", _stat.st_uid);
  req->set_header(meta_prefix + "s3fuse-uid", buf);

  snprintf(buf, 16, "%i", _stat.st_gid);
  req->set_header(meta_prefix + "s3fuse-gid", buf);

  snprintf(buf, 16, "%li", _stat.st_mtime);
  req->set_header(meta_prefix + "s3fuse-mtime", buf);

  req->set_header(meta_prefix + "s3fuse-mtime-etag", _mtime_etag);
  req->set_header(meta_prefix + "s3fuse-md5", _md5);
  req->set_header(meta_prefix + "s3fuse-md5-etag", _md5_etag);
  req->set_header("Content-Type", _content_type);
}

int object::commit_metadata(const request::ptr &req)
{
  req->init(HTTP_PUT);
  req->set_url(_url);
  req->set_header(service::get_header_prefix() + "copy-source", _url);
  req->set_header(service::get_header_prefix() + "copy-source-if-match", get_etag()); // get_etag() locks the metadata mutex
  req->set_header(service::get_header_prefix() + "metadata-directive", "REPLACE");
  req->set_meta_headers(shared_from_this());

  req->run();

  if (req->get_response_code() != HTTP_SC_OK) {
    S3_LOG(LOG_WARNING, "object::commit_metadata", "failed to commit object metadata for [%s].\n", _url.c_str());
    return -EIO;
  }

  return 0;
}

void object::adjust_stat(struct stat *s)
{
}
