/*
 * services/gs/versioning.cc
 * -------------------------------------------------------------------------
 * Object versioning implementation for Google Storage.
 * -------------------------------------------------------------------------
 *
 * Copyright (c) 2019, Tarick Bedeir.
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

#include "services/gs/versioning.h"

#include "base/logger.h"
#include "base/request.h"
#include "base/url.h"
#include "base/xml.h"
#include "services/gs/impl.h"
#include "services/utils.h"

namespace s3 {
namespace services {
namespace gs {

namespace {
constexpr char VERSION_XPATH[] = "/ListBucketResult/Version";

// echo -n "" | md5sum
constexpr char EMPTY_VERSION_ETAG[] = "\"d41d8cd98f00b204e9800998ecf8427e\"";

const std::string VERSION_ID_HEADER = "x-goog-generation";
}  // namespace

Versioning::Versioning(Impl *service) : service_(service) {}

std::string Versioning::BuildVersionedUrl(const std::string &base_path,
                                          const std::string &version) {
  return service_->bucket_url() + "/" + base::Url::Encode(base_path) +
         "?generation=" + version;
}

std::string Versioning::ExtractCurrentVersion(base::Request *req) {
  const auto version_iter = req->response_headers().find(VERSION_ID_HEADER);
  return (version_iter == req->response_headers().end()) ? ""
                                                         : version_iter->second;
}

int Versioning::FetchAllVersions(VersionFetchOptions options, std::string path,
                                 base::Request *req, std::string *out,
                                 int *empty_count) {
  req->Init(base::HttpMethod::GET);
  req->SetUrl(service_->bucket_url() + "?versions",
              std::string("prefix=") + base::Url::Encode(path));
  req->Run();

  if (req->response_code() != base::HTTP_SC_OK) return -EIO;

  auto doc = base::XmlDocument::Parse(req->GetOutputAsString());
  if (!doc) {
    S3_LOG(LOG_WARNING, "Versioning::FetchAllVersions",
           "failed to parse response.\n");
    return -EIO;
  }

  std::list<std::map<std::string, std::string>> versions;
  doc->Find(VERSION_XPATH, &versions);
  std::string versions_str;
  std::string latest_etag;

  for (auto &keys : versions) {
    const std::string &key = keys["Key"];
    if (key != path) continue;
    const std::string &etag = keys["ETag"];
    if (etag == latest_etag) continue;
    if (options != VersionFetchOptions::WITH_EMPTIES &&
        etag == EMPTY_VERSION_ETAG && empty_count != nullptr) {
      (*empty_count)++;
      continue;
    }
    latest_etag = etag;

    const auto size = FindOrDefault(keys, "Size");
    versions_str += "version=" + FindOrDefault(keys, "Generation") +
                    " mtime=" + FindOrDefault(keys, "LastModified") +
                    " etag=" + FindOrDefault(keys, "ETag") +
                    (size == "0" ? " deleted" : std::string(" size=") + size) +
                    "\n";
  }

  *out = versions_str;
  return 0;
}

}  // namespace gs
}  // namespace services
}  // namespace s3
