/*
 * services/service.h
 * -------------------------------------------------------------------------
 * Static methods for service-specific settings.
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

#ifndef S3_SERVICES_SERVICE_H
#define S3_SERVICES_SERVICE_H

#include <memory>
#include <string>

#include "services/impl.h"

namespace s3 {
namespace services {
class Service {
 public:
  static void Init(std::unique_ptr<Impl> impl);

  // Uses whatever service is defined in the config file (or the default service
  // if only one service is enabled).
  static void Init();

  static std::string GetEnabledServices();

  inline static std::string header_prefix() { return s_impl->header_prefix(); }
  inline static std::string header_meta_prefix() {
    return s_impl->header_meta_prefix();
  }

  inline static std::string bucket_url() { return s_impl->bucket_url(); }

  inline static bool is_next_marker_supported() {
    return s_impl->is_next_marker_supported();
  }

  inline static FileTransfer *file_transfer() { return s_file_transfer.get(); }

 private:
  static std::unique_ptr<Impl> s_impl;
  static std::unique_ptr<FileTransfer> s_file_transfer;
};
}  // namespace services
}  // namespace s3

#endif
