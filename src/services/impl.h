/*
 * services/impl.h
 * -------------------------------------------------------------------------
 * Base class for service-specific implementation classes.
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

#ifndef S3_SERVICES_IMPL_H
#define S3_SERVICES_IMPL_H

#include <string>

namespace s3 {
namespace base {
class RequestHook;
}

namespace services {
class FileTransfer;

class Impl {
 public:
  virtual ~Impl() = default;

  virtual std::string header_prefix() const = 0;
  virtual std::string header_meta_prefix() const = 0;
  virtual std::string bucket_url() const = 0;

  virtual bool is_next_marker_supported() const = 0;

  virtual base::RequestHook *hook() = 0;
  virtual FileTransfer *file_transfer() = 0;
};
}  // namespace services
}  // namespace s3

#endif
