/*
 * services/gs/impl.cc
 * -------------------------------------------------------------------------
 * Definitions for Google Storage implementation.
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

#include <atomic>
#include <memory>
#include <mutex>
#include <sstream>

#include <json/json.h>

#include "base/config.h"
#include "base/logger.h"
#include "base/request.h"
#include "base/statistics.h"
#include "crypto/private_file.h"
#include "services/gs/file_transfer.h"
#include "services/gs/impl.h"

using std::atomic_int;
using std::mutex;
using std::shared_ptr;
using std::endl;
using std::ifstream;
using std::lock_guard;
using std::mutex;
using std::ofstream;
using std::ostream;
using std::runtime_error;
using std::string;
using std::stringstream;

using s3::base::config;
using s3::base::request;
using s3::base::statistics;
using s3::crypto::private_file;
using s3::services::file_transfer;
using s3::services::gs::impl;

namespace
{
  const string HEADER_PREFIX = "x-goog-";
  const string HEADER_META_PREFIX = "x-goog-meta-";
  const string URL_PREFIX = "https://commondatastorage.googleapis.com";

  const string EP_TOKEN = "https://accounts.google.com/o/oauth2/token";
  const string OAUTH_SCOPE = "https%3a%2f%2fwww.googleapis.com%2fauth%2fdevstorage.full_control";

  const string CLIENT_ID = "591551582755.apps.googleusercontent.com";
  const string CLIENT_SECRET = "CQAaXZWfWJKdy_IV7TNZfO1P";

  const string NEW_TOKEN_URL = 
    "https://accounts.google.com/o/oauth2/auth?"
    "client_id=" + CLIENT_ID + "&"
    "redirect_uri=urn%3aietf%3awg%3aoauth%3a2.0%3aoob&"
    "scope=" + OAUTH_SCOPE + "&"
    "response_type=code";

  atomic_int s_refresh_on_fail(0), s_refresh_on_expiry(0);

  void statistics_writer(ostream *o)
  {
    *o <<
      "google storage service:\n"
      "  token refreshes due to request failure: " << s_refresh_on_fail << "\n"
      "  token refreshes due to expiry: " << s_refresh_on_expiry << "\n";
  }

  statistics::writers::entry s_entry(statistics_writer, 0);
}

const string & impl::get_new_token_url()
{
  return NEW_TOKEN_URL;
}

void impl::get_tokens(get_tokens_mode mode, const string &key, string *access_token, time_t *expiry, string *refresh_token)
{
  request req;
  string data;
  stringstream ss;
  Json::Value tree;

  data =
    "client_id=" + CLIENT_ID + "&"
    "client_secret=" + CLIENT_SECRET + "&";

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

  req.init(base::HTTP_POST);
  req.set_url(EP_TOKEN);
  req.set_input_buffer(data);

  req.run();

  if (req.get_response_code() != base::HTTP_SC_OK) {
    S3_LOG(LOG_ERR, "impl::get_tokens", "token endpoint returned %i.\n", req.get_response_code());
    throw runtime_error("failed to get tokens.");
  }

  ss << req.get_output_string();
  ss >> tree;

  if (!tree.isMember("access_token") || !tree.isMember("expires_in") ||
      !tree.isMember("refresh_token")) {
    throw runtime_error("failed to parse response.");
  }

  *access_token = tree["access_token"].asString();
  *expiry = time(NULL) + tree["expires_in"].asInt();

  if (mode == GT_AUTH_CODE)
    *refresh_token = tree["refresh_token"].asString();
}

string impl::read_token(const string &file)
{
  ifstream f;
  string token;

  private_file::open(file, &f);
  getline(f, token);

  return token;
}

void impl::write_token(const string &file, const string &token)
{
  ofstream f;

  private_file::open(file, &f, private_file::OM_OVERWRITE);
  f << token << endl;
}

impl::impl()
  : _expiry(0)
{
  lock_guard<mutex> lock(_mutex);

  _bucket_url = string("/") + request::url_encode(config::get_bucket_name());

  _refresh_token = impl::read_token(config::get_gs_token_file());
  refresh(lock);
}

const string & impl::get_header_prefix()
{
  return HEADER_PREFIX;
}

const string & impl::get_header_meta_prefix()
{
  return HEADER_META_PREFIX;
}

const string & impl::get_bucket_url()
{
  return _bucket_url;
}

bool impl::is_next_marker_supported()
{
  return true;
}

void impl::sign(request *req, int iter)
{
  lock_guard<mutex> lock(_mutex);

  if (iter > 0) {
    ++s_refresh_on_fail;
    S3_LOG(LOG_DEBUG, "impl::sign", "last request failed. refreshing token.\n");
    refresh(lock);
  } else if (time(NULL) >= _expiry) {
    ++s_refresh_on_expiry;
    S3_LOG(LOG_DEBUG, "impl::sign", "token expired. refreshing.\n");
    refresh(lock);
  }

  req->set_header("Authorization", _access_token);
  req->set_header("x-goog-api-version", "2");

  if (!config::get_gs_project_id().empty())
    req->set_header("x-goog-project-id", config::get_gs_project_id());
}

void impl::refresh(const lock_guard<mutex> &lock)
{
  impl::get_tokens(
    GT_REFRESH, 
    _refresh_token, 
    &_access_token,
    &_expiry,
    NULL);

  S3_LOG(
    LOG_DEBUG, 
    "impl::refresh", 
    "using refresh token [%s], got access token [%s].\n",
    _refresh_token.c_str(),
    _access_token.c_str());

  _access_token = "OAuth " + _access_token;
}

string impl::adjust_url(const string &url)
{
  return URL_PREFIX + url;
}

void impl::pre_run(request *r, int iter)
{
  sign(r, iter);
}

bool impl::should_retry(request *r, int iter)
{
  if (services::impl::should_retry(r, iter))
    return true;

  // retry only on first unauthorized response
  return (r->get_response_code() == base::HTTP_SC_UNAUTHORIZED && iter == 0);
}

shared_ptr<file_transfer> impl::build_file_transfer()
{
  return shared_ptr<file_transfer>(new gs::file_transfer());
}
