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

using boost::lexical_cast;
using std::string;

using s3::base::request;
using s3::base::xml;
using s3::fs::callback_xattr;
using s3::fs::glacier;
using s3::fs::object;
using s3::fs::xattr;
using s3::services::service;

namespace
{
  const char *STORAGE_CLASS_XPATH = "/s3:ListBucketResult/s3:Contents/s3:StorageClass";

  string unformat(const string &s)
  {
    string r;
    size_t pos = s.find('=');

    if (s.empty())
      return s;

    if (pos == string::npos) {
      S3_LOG(LOG_WARNING, "unformat", "malformed string: [%s]\n", s.c_str());
      return "";
    }

    r = s.substr(pos + 1);

    if (*r.begin() != '"' || *(r.end() - 1) != '"') {
      S3_LOG(LOG_WARNING, "unformat", "missing quotes: [%s]\n", s.c_str());
      return "";
    }

    return r.substr(1, r.size() - 2);
  }

  inline void set_mode(const xattr::ptr &x, int mode, bool enable)
  {
    x->set_mode((x->get_mode() & ~mode) | (enable ? mode : 0));
  }

  int get_nop_callback(const char *value, string *out)
  {
    *out = value;

    return 0;
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

  _restore_ongoing_xattr = callback_xattr::create(
    "s3fuse_restore_ongoing",
    bind(&glacier::get_restore_ongoing_value, this, _1),
    set_nop_callback,
    xattr::XM_DEFAULT);

  _restore_expiry_xattr = callback_xattr::create(
    "s3fuse_restore_expiry",
    bind(&glacier::get_restore_expiry_value, this, _1),
    set_nop_callback,
    xattr::XM_DEFAULT);

  _request_restore_xattr = callback_xattr::create(
    "s3fuse_request_restore",
    bind(get_nop_callback, "set-to-num-days-for-restore", _1),
    bind(&glacier::set_request_restore_value, this, _1),
    xattr::XM_VISIBLE | xattr::XM_WRITABLE);
}

int glacier::query_storage_class(const request::ptr &req)
{
  xml::document doc;

  req->init(base::HTTP_GET);
  req->set_url(service::get_bucket_url(), string("max-keys=1&prefix=") + request::url_encode(_object->get_path()));
  req->run();

  if (req->get_response_code() != base::HTTP_SC_OK)
    return -EIO;

  doc = xml::parse(req->get_output_string());

  if (!doc) {
    S3_LOG(LOG_WARNING, "glacier::query_storage_class", "failed to parse response.\n");
    return -EIO;
  }

  xml::find(doc, STORAGE_CLASS_XPATH, &_storage_class);

  if (_storage_class.empty()) {
    S3_LOG(LOG_WARNING, "glacier::query_storage_class", "cannot find storage class.\n");
    return -EIO;
  }

  return 0;
}

void glacier::read_restore_status(const request::ptr &req)
{
  const string &restore = req->get_response_header("x-amz-restore");

  _restore_ongoing.clear();
  _restore_expiry.clear();

  if (!restore.empty()) {
    size_t pos;

    pos = restore.find(',');

    if (pos == string::npos) {
      _restore_ongoing = restore;
    } else {
      _restore_ongoing = restore.substr(0, pos);
      _restore_expiry = restore.substr(pos + 2);
    }

    _restore_ongoing = unformat(_restore_ongoing);
    _restore_expiry = unformat(_restore_expiry);

    if (_restore_ongoing.empty()) {
      S3_LOG(LOG_WARNING, "glacier::read_restore_status", "malformed ongoing status string: [%s]\n", restore.c_str());
      _restore_ongoing = "error";
    } else if (_restore_ongoing == "false" && _restore_expiry.empty()) {
      S3_LOG(LOG_WARNING, "glacier::read_restore_status", "empty expiry when ongoing is false: [%s]\n", restore.c_str());
      _restore_ongoing = "error";
    }
  }

  set_mode(_restore_ongoing_xattr, xattr::XM_VISIBLE, !_restore_ongoing.empty());
  set_mode(_restore_expiry_xattr, xattr::XM_VISIBLE, !_restore_expiry.empty());
  set_mode(_request_restore_xattr, xattr::XM_VISIBLE, _restore_ongoing != "true");
}

int glacier::start_restore(const request::ptr &req, int days)
{
  if (_restore_ongoing == "true") {
    S3_LOG(
      LOG_DEBUG, 
      "glacier::start_restore", 
      "attempted to start restore when restore is ongoing on [%s]\n", 
      _object->get_path().c_str());

    return 0;
  }

  req->init(base::HTTP_POST);
  req->set_url(_object->get_url() + "?restore");
  req->set_header("Content-Type", "");

  req->set_input_buffer(
    string("<RestoreRequest><Days>") + 
    lexical_cast<string>(days) +
    "</Days></RestoreRequest>");

  req->run();

  if (req->get_response_code() != base::HTTP_SC_OK && req->get_response_code() != base::HTTP_SC_ACCEPTED) {
    S3_LOG(
      LOG_WARNING,
      "glacier::start_restore",
      "restore request failed for [%s] with status %i\n",
      _object->get_path().c_str(),
      req->get_response_code());

    return -EIO;
  }

  req->init(base::HTTP_HEAD);
  req->set_url(_object->get_url());
  req->run();

  if (req->get_response_code() != base::HTTP_SC_OK) {
    S3_LOG(
      LOG_WARNING,
      "glacier::start_restore",
      "failed to retrieve object metadata for [%s] with status %i\n",
      _object->get_path().c_str(),
      req->get_response_code());

    return -EIO;
  }

  read_restore_status(req);

  return 0;
}
