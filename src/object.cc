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
#include "logger.h"
#include "object.h"
#include "request.h"
#include "service.h"

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

  const string  META_BASE64_PREFIX            = "__s3fuse_base64__ ";

  const char   *META_BASE64_PREFIX_CSTR       = META_BASE64_PREFIX.c_str();
  const size_t  META_BASE64_PREFIX_LEN        = META_BASE64_PREFIX.size();

  const char   *SYMLINK_CONTENT_TYPE          = "text/symlink";

  mode_t get_mode_by_type(object_type type)
  {
    if (type == OT_FILE)
      return S_IFREG;

    if (type == OT_DIRECTORY)
      return S_IFDIR;

    if (type == OT_SYMLINK)
      return S_IFLNK;

    return 0;
  }
}

class object::meta_value
{
public:
  static inline meta_value_ptr from_string(const string &value)
  {
    meta_value_ptr ptr(new meta_value());

    if (!util::is_valid_http_string(value))
      throw runtime_error("cannot set value to string that isn't correctly encoded");

    ptr->_has_string_representation = true;
    ptr->_value.resize(value.size() + 1);
    memcpy(&ptr->_value[0], reinterpret_cast<const uint8_t *>(value.c_str()), value.size() + 1);

    return ptr;
  }

  static inline meta_value_ptr from_header(const string &value)
  {
    if (strncmp(value.c_str(), META_BASE64_PREFIX_CSTR, META_BASE64_PREFIX_LEN) == 0) {
      meta_value_ptr ptr(new meta_value());

      util::base64_decode(value.substr(META_BASE64_PREFIX_LEN), &ptr->_value);

      return ptr;
    } else 
      return from_string(value);
  }

  inline meta_value()
    : _has_string_representation(false)
  {
  }

  inline void set(const char *value, size_t size)
  {
    _value.resize(size);
    memcpy(&_value[0], reinterpret_cast<const uint8_t *>(value), size);

    if (value[size] == '\0' && strlen(value) == size && util::is_valid_http_string(string(value)))
      _has_string_representation = true;
  }

  inline int get(char *buffer, size_t max_size)
  {
    if (buffer == NULL || _value.size() > max_size)
      return _value.size();

    memcpy(reinterpret_cast<uint8_t *>(buffer), &_value[0], _value.size());
    return 0;
  }

  inline string to_header()
  {
    if (_has_string_representation)
      return string(reinterpret_cast<char *>(&_value[0]));
    else
      return META_BASE64_PREFIX + util::base64_encode(&_value[0], _value.size());
  }

private:
  bool _has_string_representation;
  vector<uint8_t> _value;
};

string object::get_bucket_url()
{
  return "/" + util::url_encode(config::get_bucket_name());
}

string object::build_url(const string &path, object_type type)
{
  return get_bucket_url() + "/" + util::url_encode(path) + ((type == OT_DIRECTORY) ? "/" : "");
}

object::object(const string &path, object_type type)
  : _type(type),
    _path(path),
    _expiry(0)
{
  init_stat();

  _stat.st_mode |= get_mode_by_type(type);
  _stat.st_mtime = time(NULL);

  _content_type = ((type == OT_SYMLINK) ? string(SYMLINK_CONTENT_TYPE) : config::get_default_content_type());
  _expiry = time(NULL) + config::get_cache_expiry_in_s();
  _url = build_url(_path, _type);
}

void object::init_stat()
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
}

int object::set_metadata(const string &key, const char *value, size_t size, int flags)
{
  mutex::scoped_lock lock(_metadata_mutex);
  meta_map::iterator itor = _metadata.find(key);

  if (strncmp(key.c_str(), META_PREFIX_RESERVED_CSTR, META_PREFIX_RESERVED_LEN) == 0)
    return -EINVAL;

  if (key == "__md5__" || key == "__etag__" || key == "__content_type__")
    return -EINVAL;

  if (flags & XATTR_CREATE && itor != _metadata.end())
    return -EEXIST;

  if (itor == _metadata.end()) {
    if (flags & XATTR_REPLACE)
      return -ENODATA;

    itor = _metadata.insert(make_pair(key, meta_value_ptr(new meta_value()))).first;
  }

  itor->second->set(value, size);

  return 0;
}

void object::get_metadata_keys(vector<string> *keys)
{
  mutex::scoped_lock lock(_metadata_mutex);

  keys->push_back("__md5__");
  keys->push_back("__etag__");
  keys->push_back("__content_type__");

  for (meta_map::const_iterator itor = _metadata.begin(); itor != _metadata.end(); ++itor)
    keys->push_back(itor->first);
}

int object::get_metadata(const string &key, char *buffer, size_t max_size)
{
  mutex::scoped_lock lock(_metadata_mutex);
  meta_value_ptr value;

  if (key == "__md5__")
    value = meta_value::from_string(_md5);
  else if (key == "__etag__")
    value = meta_value::from_string(_etag);
  else if (key == "__content_type__")
    value = meta_value::from_string(_content_type);
  else {
    meta_map::const_iterator itor = _metadata.find(key);

    if (itor != _metadata.end())
      value = itor->second;
  }

  return value ? value->get(buffer, max_size) : -ENODATA;
}

int object::remove_metadata(const string &key)
{
  mutex::scoped_lock lock(_metadata_mutex);
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

void object::request_init()
{
  init_stat();

  _type = OT_INVALID;
  _content_type.clear();
  _etag.clear();
  _mtime_etag.clear();
  _md5.clear();
  _md5_etag.clear();
  _expiry = 0;
  _metadata.clear();
  _url.clear();
}

void object::request_process_header(const string &key, const string &value)
{
  // this doesn't need to lock the metadata mutex because the object won't be in the cache (and thus
  // isn't shareable) until the request has finished processing

  long long_value = strtol(value.c_str(), NULL, 0);
  string meta_prefix = service::get_header_prefix() + META_PREFIX;

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
  else if (key == (meta_prefix + "s3fuse-md5"))
    _md5 = value;
  else if (key == (meta_prefix + "s3fuse-md5-etag"))
    _md5_etag = value;
  else if (
    strncmp(key.c_str(), meta_prefix.c_str(), meta_prefix.size()) == 0 &&
    strncmp(key.c_str() + meta_prefix.size(), META_PREFIX_RESERVED_CSTR, META_PREFIX_RESERVED_LEN) != 0
  )
    _metadata[key.substr(meta_prefix.size())] = meta_value::from_header(value);
}

void object::request_process_response(request *req)
{
  // see note in request_process_header() re. locking

  const string &url = req->get_url();

  if (url.empty() || req->get_response_code() != HTTP_SC_OK)
    return;

  if (url[url.size() - 1] == '/')
    _type = OT_DIRECTORY;
  else if (_content_type == SYMLINK_CONTENT_TYPE)
    _type = OT_SYMLINK;
  else
    _type = OT_FILE;

  _url = build_url(_path, _type);

  // override UID?
  if (static_cast<uid_t>(config::get_override_uid()) != UID_MAX)
    _stat.st_uid = config::get_override_uid();

  // override GID?
  if (static_cast<gid_t>(config::get_override_gid()) != GID_MAX)
    _stat.st_gid = config::get_override_gid();

  // override mode?
  if (config::get_override_mode() != 0)
    _stat.st_mode = config::get_override_mode();

  _stat.st_mode  |= get_mode_by_type(_type);

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

  if (_type == OT_FILE)
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
  for (meta_map::const_iterator itor = _metadata.begin(); itor != _metadata.end(); ++itor)
    req->set_header(meta_prefix + itor->first, itor->second->to_header());

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
