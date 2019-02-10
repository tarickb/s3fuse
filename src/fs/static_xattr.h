/*
 * fs/static_xattr.h
 * -------------------------------------------------------------------------
 * Represents a static object extended attribute.
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

#ifndef S3_FS_STATIC_XATTR_H
#define S3_FS_STATIC_XATTR_H

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "fs/xattr.h"

namespace s3 {
namespace fs {
class StaticXAttr : public XAttr {
 public:
  static std::unique_ptr<StaticXAttr> FromHeader(
      const std::string &header_key, const std::string &header_value, int mode);

  static std::unique_ptr<StaticXAttr> FromString(const std::string &key,
                                                 const std::string &value,
                                                 int mode);

  static std::unique_ptr<StaticXAttr> Create(const std::string &key, int mode);

  ~StaticXAttr() override = default;

  int SetValue(const char *value, size_t size) override;
  int GetValue(char *buffer, size_t max_size) const override;

  void ToHeader(std::string *header, std::string *value) const override;
  std::string ToString() const override;

 private:
  inline StaticXAttr(const std::string &key, bool encode_key, bool encode_value,
                     int mode)
      : XAttr(key, mode),
        encode_key_(encode_key),
        encode_value_(encode_value),
        hide_on_empty_(mode & XM_VISIBLE) {}

  std::vector<uint8_t> value_;
  bool encode_key_, encode_value_;
  bool hide_on_empty_;
};
}  // namespace fs
}  // namespace s3

#endif
