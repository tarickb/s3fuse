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

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "base/request.h"
#include "fs/metadata.h"
#include "fs/special.h"
#include "services/service.h"

namespace s3 {
namespace fs {

namespace {
const std::string CONTENT_TYPE = "binary/s3fuse-special_0100"; // version 1.0

object *checker(const std::string &path, const base::request::ptr &req) {
  if (req->get_response_header("Content-Type") != CONTENT_TYPE)
    return NULL;

  return new special(path);
}

object::type_checker_list::entry s_checker_reg(checker, 100);
} // namespace

special::special(const std::string &path) : object(path) {
  set_content_type(CONTENT_TYPE);
}

special::~special() {}

void special::init(const base::request::ptr &req) {
  const std::string &meta_prefix = services::service::get_header_meta_prefix();
  mode_t mode;
  dev_t dev;

  object::init(req);

  mode = strtol(
      req->get_response_header(meta_prefix + metadata::FILE_TYPE).c_str(), NULL,
      0);

  // see note in set_request_headers()
  dev = static_cast<dev_t>(
      strtoull(req->get_response_header(meta_prefix + metadata::DEVICE).c_str(),
               NULL, 0));

  set_type(mode);
  set_device(dev);
}

void special::set_request_headers(const base::request::ptr &req) {
  const std::string &meta_prefix = services::service::get_header_meta_prefix();
  char buf[16];

  object::set_request_headers(req);

  snprintf(buf, 16, "%#o", get_stat()->st_mode & S_IFMT);
  req->set_header(meta_prefix + metadata::FILE_TYPE, buf);

  // dev_t is int32_t on OS X and uint64_t on Linux, so generalize
  snprintf(buf, 16, "%" PRIu64, static_cast<uint64_t>(get_stat()->st_rdev));
  req->set_header(meta_prefix + metadata::DEVICE, buf);
}

} // namespace fs
} // namespace s3
