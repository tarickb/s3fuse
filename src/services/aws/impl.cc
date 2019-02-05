/*
 * services/aws/impl.cc
 * -------------------------------------------------------------------------
 * Definitions for AWS service implementation.
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

#include <iterator>
#include <sstream>
#include <vector>

#include "base/config.h"
#include "base/logger.h"
#include "base/request.h"
#include "base/timer.h"
#include "crypto/base64.h"
#include "crypto/encoder.h"
#include "crypto/hmac_sha1.h"
#include "crypto/private_file.h"
#include "services/aws/file_transfer.h"
#include "services/aws/impl.h"

namespace s3 {
namespace services {
namespace aws {

namespace {
const std::string HEADER_PREFIX = "x-amz-";
const std::string HEADER_META_PREFIX = "x-amz-meta-";

const std::string EMPTY = "";

const std::string &safe_find(const base::header_map &map, const char *key) {
  base::header_map::const_iterator itor = map.find(key);

  return (itor == map.end()) ? EMPTY : itor->second;
}
} // namespace

impl::impl() {
  std::ifstream f;
  std::string line;

  crypto::private_file::open(base::config::get_aws_secret_file(), &f);
  getline(f, line);

  std::istringstream line_stream(line);
  std::vector<std::string> fields{
      std::istream_iterator<std::string>(line_stream),
      std::istream_iterator<std::string>()};

  if (fields.size() != 2) {
    S3_LOG(LOG_ERR, "impl::impl",
           "expected 2 fields for aws_secret_file, found %i.\n", fields.size());

    throw std::runtime_error("error while parsing auth data for AWS.");
  }

  _key = fields[0];
  _secret = fields[1];

  _endpoint = base::config::get_aws_use_ssl() ? "https://" : "http://";
  _endpoint += base::config::get_aws_service_endpoint();

  _bucket_url = std::string("/") +
                base::request::url_encode(base::config::get_bucket_name());
}

const std::string &impl::get_header_prefix() { return HEADER_PREFIX; }

const std::string &impl::get_header_meta_prefix() { return HEADER_META_PREFIX; }

const std::string &impl::get_bucket_url() { return _bucket_url; }

bool impl::is_next_marker_supported() { return true; }

void impl::sign(base::request *req) {
  const base::header_map &headers = req->get_headers();
  std::string date, to_sign;
  uint8_t mac[crypto::hmac_sha1::MAC_LEN];

  date = base::timer::get_http_time();

  req->set_header("Date", date);

  to_sign = req->get_method() + "\n" + safe_find(headers, "Content-MD5") +
            "\n" + safe_find(headers, "Content-Type") + "\n" + date + "\n";

  for (base::header_map::const_iterator itor = headers.begin();
       itor != headers.end(); ++itor)
    if (!itor->second.empty() &&
        itor->first.substr(0, HEADER_PREFIX.size()) == HEADER_PREFIX)
      to_sign += itor->first + ":" + itor->second + "\n";

  to_sign += req->get_url();

  crypto::hmac_sha1::sign(_secret, to_sign, mac);
  req->set_header("Authorization", std::string("AWS ") + _key + ":" +
                                       crypto::encoder::encode<crypto::base64>(
                                           mac, crypto::hmac_sha1::MAC_LEN));
}

std::string impl::adjust_url(const std::string &url) { return _endpoint + url; }

void impl::pre_run(base::request *r, int iter) { sign(r); }

std::shared_ptr<services::file_transfer> impl::build_file_transfer() {
  return std::shared_ptr<file_transfer>(new aws::file_transfer());
}

} // namespace aws
} // namespace services
} // namespace s3
