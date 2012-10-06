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
#include "fs.h"
#include "request.h"
#include "service.h"
#include "util.h"
#include "xml.h"

using namespace boost;
using namespace std;

using namespace s3;

namespace
{
  const string SYMLINK_PREFIX = "SYMLINK:";
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

// TODO: invalidate parent when removing an object!
