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

namespace s3 {
  namespace services {
    namespace gs {

namespace
{
  const std::string HEADER_PREFIX = "x-goog-";
  const std::string HEADER_META_PREFIX = "x-goog-meta-";
  const std::string URL_PREFIX = "https://commondatastorage.googleapis.com";

  const std::string EP_TOKEN = "https://accounts.google.com/o/oauth2/token";
  const std::string OAUTH_SCOPE = "https%3a%2f%2fwww.googleapis.com%2fauth%2fdevstorage.full_control";

  const std::string CLIENT_ID = "591551582755.apps.googleusercontent.com";
  const std::string CLIENT_SECRET = "CQAaXZWfWJKdy_IV7TNZfO1P";

  const std::string NEW_TOKEN_URL = 
    "https://accounts.google.com/o/oauth2/auth?"
    "client_id=" + CLIENT_ID + "&"
    "redirect_uri=urn%3aietf%3awg%3aoauth%3a2.0%3aoob&"
    "scope=" + OAUTH_SCOPE + "&"
    "response_type=code";

  std::atomic_int s_refresh_on_fail(0), s_refresh_on_expiry(0);

  void statistics_writer(std::ostream *o)
  {
    *o <<
      "google storage service:\n"
      "  token refreshes due to request failure: " << s_refresh_on_fail << "\n"
      "  token refreshes due to expiry: " << s_refresh_on_expiry << "\n";
  }

  base::statistics::writers::entry s_entry(statistics_writer, 0);
}

const std::string & impl::get_new_token_url()
{
  return NEW_TOKEN_URL;
}

void impl::get_tokens(get_tokens_mode mode, const std::string &key, std::string
    *access_token, time_t *expiry, std::string *refresh_token)
{
  base::request req;
  std::string data;
  std::stringstream ss;
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
    throw std::runtime_error("unrecognized get_tokens mode.");

  req.init(base::HTTP_POST);
  req.set_url(EP_TOKEN);
  req.set_input_buffer(data);

  req.run();

  if (req.get_response_code() != base::HTTP_SC_OK) {
    S3_LOG(LOG_ERR, "impl::get_tokens", "token endpoint returned %i.\n", req.get_response_code());
    throw std::runtime_error("failed to get tokens.");
  }

  ss << req.get_output_string();
  ss >> tree;

  if (!tree.isMember("access_token") || !tree.isMember("expires_in") ||
      !tree.isMember("refresh_token")) {
    throw std::runtime_error("failed to parse response.");
  }

  *access_token = tree["access_token"].asString();
  *expiry = time(NULL) + tree["expires_in"].asInt();

  if (mode == GT_AUTH_CODE)
    *refresh_token = tree["refresh_token"].asString();
}

std::string impl::read_token(const std::string &file)
{
  std::ifstream f;
  std::string token;

  crypto::private_file::open(file, &f);
  getline(f, token);

  return token;
}

void impl::write_token(const std::string &file, const std::string &token)
{
  std::ofstream f;

  crypto::private_file::open(file, &f, crypto::private_file::OM_OVERWRITE);
  f << token << std::endl;
}

impl::impl()
  : _expiry(0)
{
  std::lock_guard<std::mutex> lock(_mutex);

  _bucket_url = std::string("/") + base::request::url_encode(base::config::get_bucket_name());

  _refresh_token = impl::read_token(base::config::get_gs_token_file());
  refresh(lock);
}

const std::string & impl::get_header_prefix()
{
  return HEADER_PREFIX;
}

const std::string & impl::get_header_meta_prefix()
{
  return HEADER_META_PREFIX;
}

const std::string & impl::get_bucket_url()
{
  return _bucket_url;
}

bool impl::is_next_marker_supported()
{
  return true;
}

void impl::sign(base::request *req, int iter)
{
  std::lock_guard<std::mutex> lock(_mutex);

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

  if (!base::config::get_gs_project_id().empty())
    req->set_header("x-goog-project-id", base::config::get_gs_project_id());
}

void impl::refresh(const std::lock_guard<std::mutex> &lock)
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

std::string impl::adjust_url(const std::string &url)
{
  return URL_PREFIX + url;
}

void impl::pre_run(base::request *r, int iter)
{
  sign(r, iter);
}

bool impl::should_retry(base::request *r, int iter)
{
  if (services::impl::should_retry(r, iter))
    return true;

  // retry only on first unauthorized response
  return (r->get_response_code() == base::HTTP_SC_UNAUTHORIZED && iter == 0);
}

std::shared_ptr<services::file_transfer> impl::build_file_transfer()
{
  return std::shared_ptr<file_transfer>(new gs::file_transfer());
}

} } }
