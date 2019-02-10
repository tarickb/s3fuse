/*
 * fs/special.h
 * -------------------------------------------------------------------------
 * Represents a "special" object (e.g., FIFO, device, etc.).
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

#ifndef S3_FS_SPECIAL_H
#define S3_FS_SPECIAL_H

#include <string>

#include "fs/object.h"

namespace s3 {
namespace fs {
class Special : public Object {
 public:
  Special(const std::string &path);
  ~Special() override = default;

  inline void set_type(mode_t mode) { Object::set_type(mode & S_IFMT); }
  inline void set_device(dev_t dev) { stat()->st_rdev = dev; }

 protected:
  void Init(base::Request *req) override;
  void SetRequestHeaders(base::Request *req) override;
};
}  // namespace fs
}  // namespace s3

#endif
