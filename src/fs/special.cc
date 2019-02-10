/*
 * fs/special.cc
 * -------------------------------------------------------------------------
 * Special object implementation.
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

#include "fs/special.h"

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "base/request.h"
#include "fs/metadata.h"
#include "services/service.h"

namespace s3 {
namespace fs {

namespace {
const std::string CONTENT_TYPE = "binary/s3fuse-special_0100";  // version 1.0

Object *Checker(const std::string &path, base::Request *req) {
  if (req->response_header("Content-Type") != CONTENT_TYPE) return nullptr;
  return new Special(path);
}

Object::TypeCheckers::Entry s_checker_reg(Checker, 100);
}  // namespace

Special::Special(const std::string &path) : Object(path) {
  set_content_type(CONTENT_TYPE);
}

void Special::Init(base::Request *req) {
  const std::string meta_prefix = services::Service::header_meta_prefix();

  Object::Init(req);

  mode_t mode =
      strtol(req->response_header(meta_prefix + Metadata::FILE_TYPE).c_str(),
             nullptr, 0);
  // see note in set_request_headers()
  dev_t dev = static_cast<dev_t>(
      strtoull(req->response_header(meta_prefix + Metadata::DEVICE).c_str(),
               nullptr, 0));

  set_type(mode);
  set_device(dev);
}

void Special::SetRequestHeaders(base::Request *req) {
  const std::string &meta_prefix = services::Service::header_meta_prefix();

  Object::SetRequestHeaders(req);

  char buf[16];
  snprintf(buf, 16, "%#o", stat()->st_mode & S_IFMT);
  req->SetHeader(meta_prefix + Metadata::FILE_TYPE, buf);

  req->SetHeader(meta_prefix + Metadata::DEVICE,
                 std::to_string(stat()->st_rdev));
}

}  // namespace fs
}  // namespace s3
