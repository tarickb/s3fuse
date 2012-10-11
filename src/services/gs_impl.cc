/*
 * gs_impl.cc
 * -------------------------------------------------------------------------
 * Definitions for Google Storage implementation.
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

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include "config.h"
#include "encoding.h"
#include "logger.h"
#include "request.h"
#include "services/gs_impl.h"

using namespace boost;
using namespace boost::property_tree;
using namespace std;

using namespace s3;
using namespace s3::crypto;
using namespace s3::services;

namespace
{
  const string GS_HEADER_PREFIX = "x-goog-";
  const string GS_HEADER_META_PREFIX = "x-goog-meta-";
  const string GS_URL_PREFIX = "https://commondatastorage.googleapis.com";
  const string GS_XML_NAMESPACE = "http://doc.s3.amazonaws.com/2006-03-01"; // "http://doc.commondatastorage.googleapis.com/2010-04-03";

  const string GS_EP_TOKEN = "https://accounts.google.com/o/oauth2/token";
  const string GS_OAUTH_SCOPE = "https%3a%2f%2fwww.googleapis.com%2fauth%2fdevstorage.read_write";

  const string GS_CLIENT_ID = "591551582755.apps.googleusercontent.com";
  const string GS_CLIENT_SECRET = "CQAaXZWfWJKdy_IV7TNZfO1P";

  const string GS_NEW_TOKEN_URL = 
    "https://accounts.google.com/o/oauth2/auth?"
    "client_id=" + GS_CLIENT_ID + "&"
    "redirect_uri=urn%3aietf%3awg%3aoauth%3a2.0%3aoob&"
    "scope=" + GS_OAUTH_SCOPE + "&"
    "response_type=code";
}

const string & gs_impl::get_new_token_url()
{
  return GS_NEW_TOKEN_URL;
}

void gs_impl::get_tokens(get_tokens_mode mode, const string &key, string *access_token, time_t *expiry, string *refresh_token)
{
  request req;
  string data;
  stringstream ss;
  ptree tree;

  data =
    "client_id=" + GS_CLIENT_ID + "&"
    "client_secret=" + GS_CLIENT_SECRET + "&";

  if (mode == GT_AUTH_CODE)
    data += 
      "code=" + key + "&"
      "redirect_uri=urn:ietf:wg:oauth:2.0:oob&"
      "grant_type=authorization_code";
  else if (mode == GT_REFRESH)
    data +=
      "refresh_token=" + key + "&"
      "grant_type=refresh_token";
  else
    throw runtime_error("unrecognized get_tokens mode.");

  req.init(HTTP_POST);
  req.set_full_url(GS_EP_TOKEN);
  req.set_input_buffer(data);
  req.disable_signing();

  req.run();

  if (req.get_response_code() != HTTP_SC_OK) {
    S3_LOG(LOG_ERR, "gs_impl::get_tokens", "token endpoint returned %i.\n", req.get_response_code());
    throw runtime_error("failed to get tokens.");
  }

  ss << req.get_output_string();
  read_json(ss, tree);

  *access_token = tree.get<string>("access_token");
  *expiry = time(NULL) + tree.get<int>("expires_in");

  if (mode == GT_AUTH_CODE)
    *refresh_token = tree.get<string>("refresh_token");
}

string gs_impl::read_token(const string &file)
{
  ifstream f;
  string token;

  open_private_file(file, &f);
  getline(f, token);

  return token;
}

void gs_impl::write_token(const string &file, const string &token)
{
  ofstream f;

  open_private_file(file, &f);
  f << token << endl;
}

gs_impl::gs_impl()
  : _expiry(0)
{
  mutex::scoped_lock lock(_mutex);

  _bucket_url = string("/") + encoding::url_encode(config::get_bucket_name());

  _refresh_token = gs_impl::read_token(config::get_auth_data());
  refresh(lock);
}

const string & gs_impl::get_header_prefix()
{
  return GS_HEADER_PREFIX;
}

const string & gs_impl::get_header_meta_prefix()
{
  return GS_HEADER_META_PREFIX;
}

const string & gs_impl::get_url_prefix()
{
  return GS_URL_PREFIX;
}

const string & gs_impl::get_xml_namespace()
{
  return GS_XML_NAMESPACE;
}

bool gs_impl::is_multipart_download_supported()
{
  return true;
}

bool gs_impl::is_multipart_upload_supported()
{
  return false;
}

const string & gs_impl::get_bucket_url()
{
  return _bucket_url;
}

void gs_impl::sign(request *req, bool last_sign_failed)
{
  mutex::scoped_lock lock(_mutex);

  if (last_sign_failed || time(NULL) >= _expiry) 
    refresh(lock);

  req->set_header("Authorization", _access_token);
  req->set_header("x-goog-api-version", "2");
}

void gs_impl::refresh(const mutex::scoped_lock &lock)
{
  gs_impl::get_tokens(
    GT_REFRESH, 
    _refresh_token, 
    &_access_token,
    &_expiry,
    NULL);

  S3_LOG(
    LOG_DEBUG, 
    "gs_impl::refresh", 
    "using refresh token [%s], got access token [%s].\n",
    _refresh_token.c_str(),
    _access_token.c_str());

  _access_token = "OAuth " + _access_token;
}
