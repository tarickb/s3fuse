/*
 * fs/glacier.cc
 * -------------------------------------------------------------------------
 * Glacier auto-archive support class implementation.
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

#include "base/logger.h"
#include "base/request.h"
#include "base/xml.h"
#include "fs/callback_xattr.h"
#include "fs/glacier.h"
#include "fs/object.h"
#include "services/service.h"

using std::string;

using s3::base::request;
using s3::base::xml;
using s3::fs::callback_xattr;
using s3::fs::glacier;
using s3::fs::object;
using s3::services::service;

namespace
{
  const char *STORAGE_CLASS_XPATH = "/s3:ListBucketResult/s3:Contents/s3:StorageClass";

  string unformat(const string &s)
  {
    string r;
    size_t pos = s.find('=');

    if (pos == string::npos) {
      S3_LOG(LOG_WARNING, "unformat", "malformed string: [%s]\n", s.c_str());
      return "";
    }

    r = s.substr(pos + 1);

    if (*r.begin() != '=' || *r.end() != '=') {
      S3_LOG(LOG_WARNING, "unformat", "missing quotes: [%s]\n", s.c_str());
      return "";
    }

    return r.substr(1, r.size() - 2);
  }

  int set_nop_callback(const string & /* ignored */)
  {
    return 0;
  }
}

glacier::glacier(const object *obj)
  : _object(obj)
{
  _storage_class_xattr = callback_xattr::create(
    "s3fuse_storage_class",
    bind(&glacier::get_storage_class_value, this, _1),
    set_nop_callback,
    xattr::XM_VISIBLE);

  _ongoing_xattr = callback_xattr::create(
    "s3fuse_restore_ongoing",
    bind(&glacier::get_ongoing_value, this, _1),
    set_nop_callback,
    xattr::XM_DEFAULT);

  _expiry_xattr = callback_xattr::create(
    "s3fuse_restore_expiry",
    bind(&glacier::get_expiry_value, this, _1),
    set_nop_callback,
    xattr::XM_DEFAULT);
}

int glacier::read_storage_class(const request::ptr &req)
{
  xml::document doc;

  req->init(base::HTTP_GET);
  req->set_url(service::get_bucket_url(), string("max-keys=1&prefix=") + request::url_encode(_object->get_path()));
  req->run();

  if (req->get_response_code() != base::HTTP_SC_OK)
    return -EIO;

  doc = xml::parse(req->get_output_string());

  if (!doc) {
    S3_LOG(LOG_WARNING, "glacier::read_storage_class", "failed to parse response.\n");
    return -EIO;
  }

  xml::find(doc, STORAGE_CLASS_XPATH, &_storage_class);

  if (_storage_class.empty()) {
    S3_LOG(LOG_WARNING, "glacier::read_storage_class", "cannot find storage class.\n");
    return -EIO;
  }

  return 0;
}

void glacier::set_restore_status(const request::ptr &req)
{
  const string &restore = req->get_response_header("x-amz-restore");

  _ongoing.clear();
  _expiry.clear();

  if (!restore.empty()) {
    size_t pos;

    pos = restore.find(',');

    if (pos == string::npos) {
      _ongoing = restore;
    } else {
      _ongoing = restore.substr(0, pos);
      _expiry = restore.substr(pos + 2);
    }

    _ongoing = unformat(_ongoing);
    _expiry = unformat(_expiry);

    if (_ongoing.empty()) {
      S3_LOG(LOG_WARNING, "glacier::set_restore_status", "malformed ongoing status string: [%s]\n", restore.c_str());
      _ongoing = "(error)";
    } else if (_ongoing == "true" && _expiry.empty()) {
      S3_LOG(LOG_WARNING, "glacier::set_restore_status", "empty expiry when ongoing is true: [%s]\n", restore.c_str());
      _ongoing = "(error)";
    }
  }

  _ongoing_xattr->set_mode((_ongoing_xattr->get_mode() & ~xattr::XM_VISIBLE) | (_ongoing.empty() ? 0 : xattr::XM_VISIBLE));
  _expiry_xattr->set_mode((_expiry_xattr->get_mode() & ~xattr::XM_VISIBLE) | (_expiry.empty() ? 0 : xattr::XM_VISIBLE));
}
