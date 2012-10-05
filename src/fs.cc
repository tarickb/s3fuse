/*
 * fs.cc
 * -------------------------------------------------------------------------
 * Core S3 file system methods.
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

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "logger.h"
#include "file_transfer.h"
#include "fs.h"
#include "request.h"
#include "service.h"
#include "util.h"
#include "xml.h"

using namespace boost;
using namespace std;

using namespace s3;

#define ASSERT_NO_TRAILING_SLASH(str) do { if ((str)[(str).size() - 1] == '/') return -EINVAL; } while (0)

namespace
{
  const string SYMLINK_PREFIX = "SYMLINK:";

  const char *         KEY_XPATH = "/s3:ListBucketResult/s3:Contents/s3:Key";
  const char * NEXT_MARKER_XPATH = "/s3:ListBucketResult/s3:NextMarker";

}

fs::fs()
{
  service::init(config::get_service());
  xml::init(service::get_xml_namespace());

  _object_cache.reset(new object_cache());
}

int fs::remove_object(const request::ptr &req, const string &url)
{
  req->init(HTTP_DELETE);
  req->set_url(url);

  req->run();

  return (req->get_response_code() == HTTP_SC_NO_CONTENT) ? 0 : -EIO;
}

typedef shared_ptr<string> string_ptr;

struct rename_operation
{
  string_ptr old_name;
  async_handle handle;
};

void fs::invalidate_parent(const string &path)
{
  if (config::get_cache_directories()) {
    string parent_path;
    size_t last_slash = path.rfind('/');

    parent_path = (last_slash == string::npos) ? "" : path.substr(0, last_slash);

    S3_LOG(LOG_DEBUG, "fs::invalidate_parent", "invalidating parent directory [%s] for [%s].\n", parent_path.c_str(), path.c_str());
    _object_cache->remove(parent_path);
  }
}

int fs::rename_children(const request::ptr &req, const string &_from, const string &_to)
{
  size_t from_len;
  string marker = "";
  bool truncated = true;
  string from, to;
  list<rename_operation> pending_renames, pending_deletes;

  if (_from.empty())
    return -EINVAL;

  from = _from + "/";
  to = _to + "/";
  from_len = from.size();

  req->init(HTTP_GET);

  while (truncated) {
    xml::document doc;
    xml::element_list keys;
    int r;

    req->set_url(object::get_bucket_url(), string("prefix=") + util::url_encode(from) + "&marker=" + marker);
    req->run();

    if (req->get_response_code() != HTTP_SC_OK)
      return -EIO;

    doc = xml::parse(req->get_response_data());

    if (!doc) {
      S3_LOG(LOG_WARNING, "fs::rename_children", "failed to parse response.\n");
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
      oper.handle = _tp_bg->post(bind(&fs::copy_file, this, _1, *oper.old_name, new_name));

      pending_renames.push_back(oper);

      S3_LOG(LOG_DEBUG, "fs::rename_children", "[%s] -> [%s]\n", full_path_cs, new_name.c_str());
    }
  }

  while (!pending_renames.empty()) {
    int r;
    rename_operation &oper = pending_renames.front();

    r = _tp_bg->wait(oper.handle);

    if (r)
      return r;

    oper.handle.reset();
    pending_deletes.push_back(oper);
    pending_renames.pop_front();
  }

  // specify OT_FILE because it doesn't transform the path
  for (list<rename_operation>::iterator itor = pending_deletes.begin(); itor != pending_deletes.end(); ++itor)
    itor->handle = _tp_bg->post(bind(&fs::remove_object, this, _1, object::build_url(*itor->old_name, OT_FILE)));

  while (!pending_deletes.empty()) {
    int r;
    const rename_operation &oper = pending_deletes.front();

    r = _tp_bg->wait(oper.handle);
    pending_deletes.pop_front();

    if (r)
      return r;
  }

  return 0;
}

int fs::__prefill_stats(const request::ptr &req, const string &path, int hints)
{
  _object_cache->get(req, path, hints);

  return 0;
}

int fs::__get_stats(const request::ptr &req, const string &path, struct stat *s, int hints)
{
  object::ptr obj;

  ASSERT_NO_TRAILING_SLASH(path);

  obj = _object_cache->get(req, path, hints);

  if (!obj)
    return -ENOENT;

  if (s)
    obj->copy_stat(s);

  return 0;
}

int fs::__rename_object(const request::ptr &req, const string &from, const string &to)
{
  int r;
  object::ptr obj, target_obj;

  ASSERT_NO_TRAILING_SLASH(from);
  ASSERT_NO_TRAILING_SLASH(to);

  obj = _object_cache->get(req, from);

  if (!obj)
    return -ENOENT;

  target_obj = _object_cache->get(req, to);

  if (target_obj) {
    if (
      (obj->get_type() == OT_DIRECTORY && target_obj->get_type() != OT_DIRECTORY) ||
      (obj->get_type() != OT_DIRECTORY && target_obj->get_type() == OT_DIRECTORY)
    )
      return -EINVAL;

    // TODO: this doesn't handle the case where "to" points to a directory
    // that isn't empty.

    r = __remove_object(req, to);

    if (r)
      return r;
  }

  invalidate_parent(from);
  invalidate_parent(to);

  if (obj->get_type() == OT_DIRECTORY)
    return rename_children(req, from, to);
  else {
    r = copy_file(req, from, to);

    if (r)
      return r;

    _object_cache->remove(from);
    return remove_object(req, obj->get_url());
  }
}

int fs::copy_file(const request::ptr &req, const string &from, const string &to)
{
  req->init(HTTP_PUT);
  req->set_url(object::build_url(to, OT_FILE));
  req->set_header(service::get_header_prefix() + "copy-source", object::build_url(from, OT_FILE));
  req->set_header(service::get_header_prefix() + "metadata-directive", "COPY");

  req->run();

  return (req->get_response_code() == HTTP_SC_OK) ? 0 : -EIO;
}

int fs::__change_metadata(const request::ptr &req, const string &path, mode_t mode, uid_t uid, gid_t gid, time_t mtime)
{
  object::ptr obj;

  ASSERT_NO_TRAILING_SLASH(path);

  obj = _object_cache->get(req, path);

  if (!obj)
    return -ENOENT;

  if (mode != mode_t(-1))
    obj->set_mode(mode);

  if (uid != uid_t(-1))
    obj->set_uid(uid);

  if (gid != gid_t(-1))
    obj->set_gid(gid);

  if (mtime != time_t(-1))
    obj->set_mtime(mtime);

  return obj->commit_metadata(req);
}

int fs::__create_object(const request::ptr &req, const string &path, object_type type, mode_t mode, uid_t uid, gid_t gid, const string &symlink_target)
{
  object::ptr obj;

  ASSERT_NO_TRAILING_SLASH(path);

  if (_object_cache->get(req, path)) {
    S3_LOG(LOG_DEBUG, "fs::__create_object", "attempt to overwrite object at path %s.\n", path.c_str());
    return -EEXIST;
  }

  invalidate_parent(path);

  obj.reset(new object(path, type));
  obj->set_mode(mode);
  obj->set_uid(uid);
  obj->set_gid(gid);

  req->init(HTTP_PUT);
  req->set_url(obj->get_url());
  req->set_meta_headers(obj);

  if (type == OT_SYMLINK)
    req->set_input_data(SYMLINK_PREFIX + symlink_target);

  req->run();

  return (req->get_response_code() == HTTP_SC_OK) ? 0 : -EIO;
}

int fs::__remove_object(const request::ptr &req, const string &path)
{
  object::ptr obj;
  int r;

  ASSERT_NO_TRAILING_SLASH(path);

  obj = _object_cache->get(req, path, HINT_NONE);

  if (!obj)
    return -ENOENT;

  if (obj->get_type() == OT_DIRECTORY && !is_directory_empty(req, obj->get_path()))
    return -ENOTEMPTY;

  r = remove_object(req, obj->get_url());

  invalidate_parent(path);
  _object_cache->remove(path);

  return r;
}

int fs::__set_attr(const request::ptr &req, const string &path, const string &name, const char *value, size_t size, int flags)
{
  object::ptr obj;
  int r;

  ASSERT_NO_TRAILING_SLASH(path);

  obj = _object_cache->get(req, path);

  if (!obj)
    return -ENOENT;

  r = obj->set_metadata(name, value, size, flags);

  if (r)
    return r;

  return obj->commit_metadata(req);
}

int fs::__remove_attr(const request::ptr &req, const string &path, const string &name)
{
  object::ptr obj;
  int r;

  ASSERT_NO_TRAILING_SLASH(path);

  obj = _object_cache->get(req, path);

  if (!obj)
    return -ENOENT;

  r = obj->remove_metadata(name);

  if (r)
    return r;

  return obj->commit_metadata(req);
}
