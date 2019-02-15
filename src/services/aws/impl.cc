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

#include "services/aws/impl.h"

#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include "base/config.h"
#include "base/logger.h"
#include "base/paths.h"
#include "base/request.h"
#include "base/timer.h"
#include "base/url.h"
#include "crypto/base64.h"
#include "crypto/encoder.h"
#include "crypto/hmac_sha1.h"
#include "crypto/private_file.h"
#include "services/aws/file_transfer.h"
#include "services/utils.h"

namespace s3 {
namespace services {
namespace aws {

namespace {
const std::string HEADER_PREFIX = "x-amz-";
const std::string HEADER_META_PREFIX = "x-amz-meta-";
}  // namespace

Impl::Impl() {
  std::ifstream f;
  std::string line;

  crypto::PrivateFile::Open(
      base::Paths::Transform(base::Config::aws_secret_file()), &f);
  std::getline(f, line);

  std::istringstream line_stream(line);
  std::vector<std::string> fields{
      std::istream_iterator<std::string>(line_stream),
      std::istream_iterator<std::string>()};

  if (fields.size() != 2) {
    S3_LOG(LOG_ERR, "Impl::Impl",
           "expected 2 fields for aws_secret_file, found %i.\n", fields.size());
    throw std::runtime_error("error while parsing auth data for AWS.");
  }

  key_ = fields[0];
  secret_ = fields[1];

  endpoint_ = base::Config::aws_use_ssl() ? "https://" : "http://";
  endpoint_ += base::Config::aws_service_endpoint();

  bucket_url_ =
      std::string("/") + base::Url::Encode(base::Config::bucket_name());

  file_transfer_.reset(new FileTransfer());
}

std::string Impl::header_prefix() const { return HEADER_PREFIX; }

std::string Impl::header_meta_prefix() const { return HEADER_META_PREFIX; }

std::string Impl::bucket_url() const { return bucket_url_; }

bool Impl::is_next_marker_supported() const { return true; }

base::RequestHook *Impl::hook() { return this; }

services::FileTransfer *Impl::file_transfer() { return file_transfer_.get(); }

std::string Impl::AdjustUrl(const std::string &url) { return endpoint_ + url; }

void Impl::PreRun(base::Request *r, int iter) { Sign(r); }

bool Impl::ShouldRetry(base::Request *r, int iter) {
  return GenericShouldRetry(r, iter);
}

void Impl::Sign(base::Request *req) {
  const std::string date = base::Timer::GetHttpTime();
  req->SetHeader("Date", date);

  const auto &headers = req->headers();
  std::string to_sign =
      req->method() + "\n" + FindOrDefault(headers, "Content-MD5") + "\n" +
      FindOrDefault(headers, "Content-Type") + "\n" + date + "\n";

  for (const auto &header : headers)
    if (!header.second.empty() &&
        header.first.substr(0, HEADER_PREFIX.size()) == HEADER_PREFIX)
      to_sign += header.first + ":" + header.second + "\n";

  to_sign += req->url();

  uint8_t mac[crypto::HmacSha1::MAC_LEN];
  crypto::HmacSha1::Sign(secret_, to_sign, mac);
  req->SetHeader("Authorization", std::string("AWS ") + key_ + ":" +
                                      crypto::Encoder::Encode<crypto::Base64>(
                                          mac, crypto::HmacSha1::MAC_LEN));
}

}  // namespace aws
}  // namespace services
}  // namespace s3
