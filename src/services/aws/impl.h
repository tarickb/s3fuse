/*
 * services/aws/impl.h
 * -------------------------------------------------------------------------
 * Service implementation for Amazon Web Services.
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

#ifndef S3_SERVICES_AWS_IMPL_H
#define S3_SERVICES_AWS_IMPL_H

#include <string>

#include "base/request_hook.h"
#include "services/aws/file_transfer.h"
#include "services/aws/versioning.h"
#include "services/impl.h"

namespace s3 {
namespace services {
namespace aws {
class Impl : public services::Impl, public base::RequestHook {
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

  // BEGIN base::RequestHook
  std::string AdjustUrl(const std::string &url) override;
  void PreRun(base::Request *r, int iter) override;
  bool ShouldRetry(base::Request *r, int iter) override;
  // END base::RequestHook

 private:
  void Sign(base::Request *req);

  std::string key_, secret_;
  std::string bucket_url_, endpoint_, strip_url_prefix_;
  std::unique_ptr<FileTransfer> file_transfer_;
  std::unique_ptr<Versioning> versioning_;
};
}  // namespace aws
}  // namespace services
}  // namespace s3

#endif
