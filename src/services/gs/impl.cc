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

#include "services/gs/impl.h"

#include <json/json.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>

#include "base/config.h"
#include "base/logger.h"
#include "base/request.h"
#include "base/statistics.h"
#include "base/url.h"
#include "crypto/private_file.h"
#include "services/gs/file_transfer.h"
#include "services/utils.h"

namespace s3 {
namespace services {
namespace gs {

namespace {
const std::string HEADER_PREFIX = "x-goog-";
const std::string HEADER_META_PREFIX = "x-goog-meta-";

const std::string URL_PREFIX = "https://commondatastorage.googleapis.com";

const std::string EP_TOKEN = "https://accounts.google.com/o/oauth2/token";

std::atomic_int s_refresh_on_fail(0), s_refresh_on_expiry(0);

void StatsWriter(std::ostream *o) {
  *o << "google storage service:\n"
        "  token refreshes due to request failure: "
     << s_refresh_on_fail
     << "\n"
        "  token refreshes due to expiry: "
     << s_refresh_on_expiry << "\n";
}

base::Statistics::Writers::Entry s_entry(StatsWriter, 0);
}  // namespace

Impl::Tokens Impl::GetTokens(GetTokensMode mode, const std::string &client_id,
                             const std::string &client_secret,
                             const std::string &key) {
  std::string data =
      "client_id=" + client_id + "&client_secret=" + client_secret + "&";

  if (mode == GetTokensMode::AUTH_CODE)
    data += "code=" + key +
            "&"
            "redirect_uri=urn:ietf:wg:oauth:2.0:oob&"
            "grant_type=authorization_code";
  else if (mode == GetTokensMode::REFRESH)
    data += "refresh_token=" + key +
            "&"
            "grant_type=refresh_token";
  else
    throw std::runtime_error("unrecognized GetTokensMode.");

  auto req = base::RequestFactory::NewNoHook();
  req->Init(base::HttpMethod::POST);
  req->SetUrl(EP_TOKEN);
  req->SetInputBuffer(data);

  req->Run();

  if (req->response_code() != base::HTTP_SC_OK) {
    S3_LOG(LOG_ERR, "Impl::GetTokens", "token endpoint returned %i.\n",
           req->response_code());
    throw std::runtime_error("failed to get tokens.");
  }

  const std::string resp = req->GetOutputAsString();
  std::stringstream ss;
  ss << resp;

  Json::Value tree;
  ss >> tree;

  if (!tree.isMember("access_token") || !tree.isMember("expires_in")) {
    S3_LOG(LOG_WARNING, "Impl::GetTokens", "parse error: %s\n", resp.c_str());
    throw std::runtime_error("response didn't contain access token or expiry.");
  }
  if (mode == GetTokensMode::AUTH_CODE && !tree.isMember("refresh_token")) {
    S3_LOG(LOG_WARNING, "Impl::GetTokens", "parse error: %s\n", resp.c_str());
    throw std::runtime_error("response didn't contain refresh token.");
  }

  Tokens tokens;
  tokens.access = "OAuth " + tree["access_token"].asString();
  tokens.expiry = time(nullptr) + tree["expires_in"].asInt();
  tokens.refresh = (mode == GetTokensMode::AUTH_CODE)
                       ? tree["refresh_token"].asString()
                       : key;
  return tokens;
}

std::string Impl::ReadToken(const std::string &file) {
  std::ifstream f;
  crypto::PrivateFile::Open(base::Paths::Transform(file), &f);

  std::string token;
  std::getline(f, token);

  return token;
}

void Impl::WriteToken(const std::string &file, const std::string &token) {
  std::ofstream f;
  crypto::PrivateFile::Open(base::Paths::Transform(file), &f,
                            crypto::PrivateFile::OpenMode::OVERWRITE);
  f << token << std::endl;
}

Impl::Impl() {
  bucket_url_ =
      std::string("/") + base::Url::Encode(base::Config::bucket_name());

  tokens_.refresh = Impl::ReadToken(base::Config::gs_token_file());
  Refresh();

  file_transfer_.reset(new FileTransfer());
  versioning_.reset(new Versioning(this));
}

std::string Impl::header_prefix() const { return HEADER_PREFIX; }

std::string Impl::header_meta_prefix() const { return HEADER_META_PREFIX; }

std::string Impl::bucket_url() const { return bucket_url_; }

bool Impl::is_next_marker_supported() const { return true; }

base::RequestHook *Impl::hook() { return this; }

services::FileTransfer *Impl::file_transfer() { return file_transfer_.get(); }

services::Versioning *Impl::versioning() { return versioning_.get(); }

std::string Impl::AdjustUrl(const std::string &url) { return URL_PREFIX + url; }

void Impl::PreRun(base::Request *r, int iter) { Sign(r, iter); }

bool Impl::ShouldRetry(base::Request *r, int iter) {
  if (GenericShouldRetry(r, iter)) return true;

  // retry only on first unauthorized response
  return (r->response_code() == base::HTTP_SC_UNAUTHORIZED && iter == 0);
}

void Impl::Sign(base::Request *req, int iter) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (iter > 0) {
    ++s_refresh_on_fail;
    S3_LOG(LOG_DEBUG, "Impl::Sign", "last request failed. refreshing token.\n");
    Refresh();
  } else if (time(nullptr) >= tokens_.expiry) {
    ++s_refresh_on_expiry;
    S3_LOG(LOG_DEBUG, "Impl::Sign", "token expired. refreshing.\n");
    Refresh();
  }

  req->SetHeader("Authorization", tokens_.access);
  req->SetHeader("x-goog-api-version", "2");

  if (!base::Config::gs_project_id().empty())
    req->SetHeader("x-goog-project-id", base::Config::gs_project_id());
}

void Impl::Refresh() {
  tokens_ =
      Impl::GetTokens(GetTokensMode::REFRESH, base::Config::gs_client_id(),
                      base::Config::gs_client_secret(), tokens_.refresh);
  S3_LOG(LOG_DEBUG, "Impl::Refresh",
         "using refresh token [%s], got access token [%s].\n",
         tokens_.refresh.c_str(), tokens_.access.c_str());
}

}  // namespace gs
}  // namespace services
}  // namespace s3
