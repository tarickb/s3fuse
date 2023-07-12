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

#include <memory>
#include <string>

#include "base/logger.h"
#include "base/request.h"
#include "base/url.h"
#include "base/xml.h"
#include "services/service.h"

namespace s3 {
namespace fs {

namespace {
constexpr char IS_TRUNCATED_XPATH[] = "/ListBucketResult/IsTruncated";
constexpr char KEY_XPATH[] = "/ListBucketResult/Contents/Key";
constexpr char NEXT_MARKER_XPATH[] = "/ListBucketResult/NextMarker";
constexpr char NEXT_CONTINUATION_TOKEN_XPATH[] =
    "/ListBucketResult/NextContinuationToken";
constexpr char PREFIX_XPATH[] = "/ListBucketResult/CommonPrefixes/Prefix";
}  // namespace

class ListReaderV1 : public ListReader {
 public:
  ListReaderV1(const std::string &prefix, bool group_common_prefixes = true,
               int max_keys = -1)
      : prefix_(prefix),
        group_common_prefixes_(group_common_prefixes),
        max_keys_(max_keys) {}

  int Read(base::Request *req, std::list<std::string> *keys,
           std::list<std::string> *prefixes) override;

 private:
  const std::string prefix_;
  const bool group_common_prefixes_;
  const int max_keys_;
  std::string marker_;
  bool truncated_ = true;
};

int ListReaderV1::Read(base::Request *req, std::list<std::string> *keys,
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
  req->SetUrl(services::Service::bucket_url() + "/", query);
  req->Run();

  if (req->response_code() != base::HTTP_SC_OK) return -EIO;

  auto doc = base::XmlDocument::Parse(req->GetOutputAsString());
  if (!doc) {
    S3_LOG(LOG_WARNING, "ListReaderV1::Read", "failed to parse response.\n");
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
    // We only expect to get a next marker if we set a delimiter.
    if (group_common_prefixes_ &&
        services::Service::is_next_marker_supported()) {
      r = doc->Find(NEXT_MARKER_XPATH, &marker_);
      if (r) return r;
    } else {
      marker_ = keys->back();
    }
  }

  return keys->size() + (prefixes ? prefixes->size() : 0);
}

class ListReaderV2 : public ListReader {
 public:
  ListReaderV2(const std::string &prefix, bool group_common_prefixes = true,
               int max_keys = -1)
      : prefix_(prefix),
        group_common_prefixes_(group_common_prefixes),
        max_keys_(max_keys) {}

  int Read(base::Request *req, std::list<std::string> *keys,
           std::list<std::string> *prefixes) override;

 private:
  const std::string prefix_;
  const bool group_common_prefixes_;
  const int max_keys_;
  std::string continuation_token_;
  bool truncated_ = true;
};

int ListReaderV2::Read(base::Request *req, std::list<std::string> *keys,
                       std::list<std::string> *prefixes) {
  if (!keys) return -EINVAL;
  keys->clear();
  if (prefixes) prefixes->clear();

  if (!truncated_) return 0;

  std::string query =
      std::string("list-type=2&prefix=") + base::Url::Encode(prefix_);
  if (!continuation_token_.empty()) {
    S3_LOG(LOG_INFO, "ListReaderV2::Read", "token: %s\n",
           continuation_token_.c_str());
    query += "&continuation-token=" + base::Url::Encode(continuation_token_);
  }
  if (group_common_prefixes_) query += "&delimiter=/";
  if (max_keys_ > 0)
    query += std::string("&max-keys=") + std::to_string(max_keys_);

  req->Init(base::HttpMethod::GET);
  req->SetUrl(services::Service::bucket_url() + "/", query);
  req->Run();

  if (req->response_code() != base::HTTP_SC_OK) return -EIO;

  auto doc = base::XmlDocument::Parse(req->GetOutputAsString());
  if (!doc) {
    S3_LOG(LOG_WARNING, "ListReaderV2::Read", "failed to parse response.\n");
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
    r = doc->Find(NEXT_CONTINUATION_TOKEN_XPATH, &continuation_token_);
    S3_LOG(LOG_INFO, "ListReaderV2::Read", "next token: %s (%d)\n",
           continuation_token_.c_str(), r);
    if (r) return r;
  }

  return keys->size() + (prefixes ? prefixes->size() : 0);
}

std::unique_ptr<ListReader> ListReader::Create(const std::string &prefix,
                                               bool group_common_prefixes,
                                               int max_keys) {
  if (services::Service::is_listobjectsv2_supported()) {
    S3_LOG(LOG_DEBUG, "ListReader::Create", "using ListObjectsV2.\n");
    return std::make_unique<ListReaderV2>(prefix, group_common_prefixes,
                                          max_keys);
  }
  return std::make_unique<ListReaderV1>(prefix, group_common_prefixes,
                                        max_keys);
}

}  // namespace fs
}  // namespace s3
