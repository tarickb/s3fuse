/*
 * services/gs/versioning.h
 * -------------------------------------------------------------------------
 * Object versioning implementation interface for Google Storage.
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

#ifndef S3_SERVICES_GS_VERSIONING_H
#define S3_SERVICES_GS_VERSIONING_H

#include "services/versioning.h"

namespace s3 {
namespace services {
namespace gs {
class Impl;

class Versioning : public services::Versioning {
 public:
  Versioning(Impl *service);

  std::string BuildVersionedUrl(const std::string &base_path,
                                const std::string &version) override;

  std::string ExtractCurrentVersion(base::Request *req) override;

  int FetchAllVersions(VersionFetchOptions options, std::string path,
                       base::Request *req, std::string *out,
                       int *empty_count) override;

 private:
  Impl *service_;
};

}  // namespace gs
}  // namespace services
}  // namespace s3

#endif
