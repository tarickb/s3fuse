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

#include <vector>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

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

using boost::is_any_of;
using boost::shared_ptr;
using boost::token_compress_on;
using std::ifstream;
using std::runtime_error;
using std::string;
using std::vector;

using s3::base::config;
using s3::base::header_map;
using s3::base::request;
using s3::base::timer;
using s3::crypto::base64;
using s3::crypto::encoder;
using s3::crypto::hmac_sha1;
using s3::crypto::private_file;
using s3::services::file_transfer;
using s3::services::aws::impl;

namespace
{
  const string HEADER_PREFIX = "x-amz-";
  const string HEADER_META_PREFIX = "x-amz-meta-";

  const string DIRECTORY_SUFFIX = "/";
  const string DIRECTORY_URL_SUFFIX = "/";

  const string EMPTY = "";

  const string & safe_find(const header_map &map, const char *key)
  {
    header_map::const_iterator itor = map.find(key);

    return (itor == map.end()) ? EMPTY : itor->second;
  }
}

impl::impl()
{
  ifstream f;
  string line;
  vector<string> fields;

  private_file::open(config::get_aws_secret_file(), &f);
  getline(f, line);

  split(fields, line, is_any_of(string(" \t")), token_compress_on);

  if (fields.size() != 2) {
    S3_LOG(
      LOG_ERR, 
      "impl::impl", 
      "expected 2 fields for aws_secret_file, found %i.\n",
      fields.size());

    throw runtime_error("error while parsing auth data for AWS.");
  }

  _key = fields[0];
  _secret = fields[1];

  _endpoint = config::get_aws_use_ssl() ? "https://" : "http://";
  _endpoint += config::get_aws_service_endpoint();

  _bucket_url = string("/") + request::url_encode(config::get_bucket_name());
}

const string & impl::get_header_prefix()
{
  return HEADER_PREFIX;
}

const string & impl::get_header_meta_prefix()
{
  return HEADER_META_PREFIX;
}

const string & impl::get_directory_suffix()
{
  return DIRECTORY_SUFFIX;
}

const string & impl::get_directory_url_suffix()
{
  return DIRECTORY_URL_SUFFIX;
}

const string & impl::get_bucket_url()
{
  return _bucket_url;
}

void impl::sign(request *req)
{
  const header_map &headers = req->get_headers();
  string date, to_sign;
  uint8_t mac[hmac_sha1::MAC_LEN];

  date = timer::get_http_time();

  req->set_header("Date", date);
 
  to_sign = 
    req->get_method() + "\n" + 
    safe_find(headers, "Content-MD5") + "\n" + 
    safe_find(headers, "Content-Type") + "\n" + 
    date + "\n";

  for (header_map::const_iterator itor = headers.begin(); itor != headers.end(); ++itor)
    if (!itor->second.empty() && itor->first.substr(0, HEADER_PREFIX.size()) == HEADER_PREFIX)
      to_sign += itor->first + ":" + itor->second + "\n";

  to_sign += req->get_url();

  hmac_sha1::sign(_secret, to_sign, mac);
  req->set_header("Authorization", string("AWS ") + _key + ":" + encoder::encode<base64>(mac, hmac_sha1::MAC_LEN));
}

string impl::adjust_url(const string &url)
{
  return _endpoint + url;
}

void impl::pre_run(request *r, int iter)
{
  sign(r);
}

shared_ptr<file_transfer> impl::build_file_transfer()
{
  return shared_ptr<file_transfer>(new aws::file_transfer());
}
