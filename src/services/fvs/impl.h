/*
 * services/fvs/impl.h
 * -------------------------------------------------------------------------
 * Service implementation for FVS.
 * -------------------------------------------------------------------------
 *
 * Copyright (c) 2013, Tarick Bedeir, Hiroyuki Kakine.
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

#ifndef S3_SERVICES_FVS_IMPL_H
#define S3_SERVICES_FVS_IMPL_H

#include <string>

#include "base/request_hook.h"
#include "services/file_transfer.h"
#include "services/impl.h"

namespace s3 {
namespace services {
namespace fvs {
class Impl : public services::Impl,
             public services::FileTransfer,
             public base::RequestHook {
 public:
  Impl();

  ~Impl() override = default;

  // BEGIN services::Impl
  std::string header_prefix() const override;
  std::string header_meta_prefix() const override;
  std::string bucket_url() const override;

  bool is_next_marker_supported() const override;

  base::RequestHook *hook() override;
  services::FileTransfer *file_transfer() override;
  services::Versioning *versioning() override;
  // END services::Impl

  // BEGIN services::FileTransfer
  size_t upload_chunk_size() override;
  // END services::FileTransfer

  // BEGIN base::RequestHook
  std::string AdjustUrl(const std::string &url) override;
  void PreRun(base::Request *r, int iter) override;
  bool ShouldRetry(base::Request *r, int iter) override;
  // END base::RequestHook

 private:
  void Sign(base::Request *req);

  std::string key_, secret_, endpoint_, bucket_url_;
};
}  // namespace fvs
}  // namespace services
}  // namespace s3

#endif
