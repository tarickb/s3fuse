/*
 * fs/symlink.h
 * -------------------------------------------------------------------------
 * Represents a symbolic link object.
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

#ifndef S3_FS_SYMLINK_H
#define S3_FS_SYMLINK_H

#include <mutex>
#include <string>

#include "fs/object.h"
#include "threads/pool.h"

namespace s3 {
namespace fs {
class Symlink : public Object {
 public:
  Symlink(const std::string &path);
  ~Symlink() override = default;

  int Read(std::string *target);
  void SetTarget(const std::string &target);

 protected:
  void SetRequestBody(base::Request *req) override;

 private:
  int DoRead(base::Request *req);

  std::mutex mutex_;
  std::string target_;
};
}  // namespace fs
}  // namespace s3

#endif
