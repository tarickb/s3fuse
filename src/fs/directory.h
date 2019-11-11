/*
 * fs/directory.h
 * -------------------------------------------------------------------------
 * Represents a directory object.
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

#ifndef S3_FS_DIRECTORY_H
#define S3_FS_DIRECTORY_H

#include <functional>
#include <string>
#include <vector>

#include "fs/object.h"

namespace s3 {
namespace fs {
class Directory : public Object {
 public:
  using Filler = std::function<void(const std::string &)>;

  static std::string BuildUrl(const std::string &path);
  static std::vector<std::string> GetInternalObjects(base::Request *req);

  explicit Directory(const std::string &path);
  ~Directory() override = default;

  int Read(const Filler &filler);

  bool IsEmpty(base::Request *req);
  bool IsEmpty();

  int Remove(base::Request *req) override;
  int Rename(base::Request *req, std::string to) override;

 private:
  int Read(base::Request *req, const Filler &filler);
};
}  // namespace fs
}  // namespace s3

#endif
