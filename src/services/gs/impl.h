/*
 * services/gs/impl.h
 * -------------------------------------------------------------------------
 * Service implementation for Google Storage for Developers.
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

#ifndef S3_SERVICES_GS_IMPL_H
#define S3_SERVICES_GS_IMPL_H

#include <mutex>
#include <string>

#include "base/request_hook.h"
#include "services/gs/file_transfer.h"
#include "services/gs/versioning.h"
#include "services/impl.h"

namespace s3 {
namespace services {
namespace gs {
enum class GetTokensMode { AUTH_CODE, REFRESH };

class Impl : public services::Impl, public base::RequestHook {
 public:
  struct Tokens {
    std::string access;
    std::string refresh;
    time_t expiry = 0;
  };

  static Tokens GetTokens(GetTokensMode mode, const std::string &client_id,
                          const std::string &client_secret,
                          const std::string &key);

  static void WriteToken(const std::string &file, const std::string &token);
  static std::string ReadToken(const std::string &file);

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
  void Sign(base::Request *req, int iter);
  void Refresh();  // Call only with mutex_ held.

  std::string bucket_url_;
  std::unique_ptr<FileTransfer> file_transfer_;
  std::unique_ptr<Versioning> versioning_;

  std::mutex mutex_;
  Tokens tokens_;  // Protected by mutex_.
};
}  // namespace gs
}  // namespace services
}  // namespace s3

#endif
