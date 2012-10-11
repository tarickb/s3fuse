/*
 * aws_impl.cc
 * -------------------------------------------------------------------------
 * Definitions for AWS service implementation.
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

#include <vector>
#include <boost/bind.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include "base/config.h"
#include "base/logger.h"
#include "base/request.h"
#include "crypto/base64.h"
#include "crypto/encoder.h"
#include "crypto/hmac_sha1.h"
#include "services/aws_impl.h"

using boost::bind;
using boost::is_any_of;
using boost::token_compress_on;
using std::ifstream;
using std::map;
using std::runtime_error;
using std::string;
using std::vector;

using s3::base::config;
using s3::base::header_map;
using s3::base::request;
using s3::crypto::base64;
using s3::crypto::encoder;
using s3::crypto::hmac_sha1;
using s3::services::aws_impl;
using s3::services::signing_function;

namespace
{
  const string AWS_HEADER_PREFIX = "x-amz-";
  const string AWS_HEADER_META_PREFIX = "x-amz-meta-";
  const string AWS_XML_NAMESPACE = "http://s3.amazonaws.com/doc/2006-03-01/";

  const string EMPTY = "";

  const string & safe_find(const header_map &map, const char *key)
  {
    header_map::const_iterator itor = map.find(key);

    return (itor == map.end()) ? EMPTY : itor->second;
  }
}

aws_impl::aws_impl()
{
  ifstream f;
  string line;
  vector<string> fields;

  open_private_file(config::get_auth_data(), &f);
  getline(f, line);

  split(fields, line, is_any_of(string(" \t")), token_compress_on);

  if (fields.size() != 2) {
    S3_LOG(
      LOG_ERR, 
      "aws_impl::aws_impl", 
      "expected 2 fields for auth_data, found %i.\n",
      fields.size());

    throw runtime_error("error while parsing auth data for AWS.");
  }

  _key = fields[0];
  _secret = fields[1];

  _endpoint = string("https://") + config::get_aws_service_endpoint();
  _bucket_url = string("/") + request::url_encode(config::get_bucket_name());

  _signing_function = bind(&aws_impl::sign, this, _1, _2);
}

const string & aws_impl::get_header_prefix()
{
  return AWS_HEADER_PREFIX;
}

const string & aws_impl::get_header_meta_prefix()
{
  return AWS_HEADER_META_PREFIX;
}

const string & aws_impl::get_url_prefix()
{
  return _endpoint;
}

const string & aws_impl::get_xml_namespace()
{
  return AWS_XML_NAMESPACE;
}

bool aws_impl::is_multipart_download_supported()
{
  return true;
}

bool aws_impl::is_multipart_upload_supported()
{
  return true;
}

const string & aws_impl::get_bucket_url()
{
  return _bucket_url;
}

const signing_function & aws_impl::get_signing_function()
{
  return _signing_function;
}

void aws_impl::sign(request *req, bool last_sign_failed)
{
  const header_map &headers = req->get_headers();
  string to_sign;
  uint8_t mac[hmac_sha1::MAC_LEN];
 
  to_sign = 
    req->get_method() + "\n" + 
    safe_find(headers, "Content-MD5") + "\n" + 
    safe_find(headers, "Content-Type") + "\n" + 
    safe_find(headers, "Date") + "\n";

  for (header_map::const_iterator itor = headers.begin(); itor != headers.end(); ++itor)
    if (!itor->second.empty() && itor->first.substr(0, AWS_HEADER_PREFIX.size()) == AWS_HEADER_PREFIX)
      to_sign += itor->first + ":" + itor->second + "\n";

  to_sign += req->get_url();

  hmac_sha1::sign(_secret, to_sign, mac);
  req->set_header("Authorization", string("AWS ") + _key + ":" + encoder::encode<base64>(mac, hmac_sha1::MAC_LEN));

  if (last_sign_failed)
    S3_LOG(LOG_DEBUG, "aws_impl::sign", "last sign failed. string to sign: [%s].\n", to_sign.c_str());
}
