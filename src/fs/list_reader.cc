/*
 * fs/list_reader.cc
 * -------------------------------------------------------------------------
 * Lists bucket objects.
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

#include "fs/list_reader.h"

#include <errno.h>

#include <string>

#include "base/logger.h"
#include "base/request.h"
#include "base/url.h"
#include "base/xml.h"
#include "services/service.h"

namespace s3 {
namespace fs {

namespace {
const char *IS_TRUNCATED_XPATH = "/ListBucketResult/IsTruncated";
const char *KEY_XPATH = "/ListBucketResult/Contents/Key";
const char *NEXT_MARKER_XPATH = "/ListBucketResult/NextMarker";
const char *PREFIX_XPATH = "/ListBucketResult/CommonPrefixes/Prefix";
}  // namespace

ListReader::ListReader(const std::string &prefix, bool group_common_prefixes,
                       int max_keys)
    : truncated_(true),
      prefix_(prefix),
      group_common_prefixes_(group_common_prefixes),
      max_keys_(max_keys) {}

int ListReader::Read(base::Request *req, std::list<std::string> *keys,
                     std::list<std::string> *prefixes) {
  if (!keys) return -EINVAL;
  keys->clear();
  if (prefixes) prefixes->clear();

  if (!truncated_) return 0;

  std::string query = std::string("prefix=") + base::Url::Encode(prefix_) +
                      "&marker=" + marker_;
  if (group_common_prefixes_) query += "&delimiter=/";
  if (max_keys_ > 0)
    query += std::string("&max-keys=") + std::to_string(max_keys_);

  req->Init(base::HttpMethod::GET);
  req->SetUrl(services::Service::bucket_url(), query);
  req->Run();

  if (req->response_code() != base::HTTP_SC_OK) return -EIO;

  auto doc = base::XmlDocument::Parse(req->GetOutputAsString());
  if (!doc) {
    S3_LOG(LOG_WARNING, "ListReader::Read", "failed to parse response.\n");
    return -EIO;
  }

  std::string is_trunc;
  int r = doc->Find(IS_TRUNCATED_XPATH, &is_trunc);
  if (r) return r;
  truncated_ = (is_trunc == "true");

  if (prefixes) {
    r = doc->Find(PREFIX_XPATH, prefixes);
    if (r) return r;
  }

  r = doc->Find(KEY_XPATH, keys);
  if (r) return r;

  if (truncated_) {
    if (services::Service::is_next_marker_supported()) {
      r = doc->Find(NEXT_MARKER_XPATH, &marker_);
      if (r) return r;
    } else {
      marker_ = keys->back();
    }
  }

  return keys->size() + (prefixes ? prefixes->size() : 0);
}

}  // namespace fs
}  // namespace s3
