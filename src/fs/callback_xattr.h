/*
 * fs/callback_xattr.h
 * -------------------------------------------------------------------------
 * Represents an object extended attribute whose value is read from and
 * written to a pair of callback functions.
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

#ifndef S3_FS_CALLBACK_XATTR_H
#define S3_FS_CALLBACK_XATTR_H

#include <functional>
#include <memory>
#include <string>

#include "fs/xattr.h"

namespace s3 {
namespace fs {
class CallbackXAttr : public XAttr {
 public:
  using GetValueCallback = std::function<int(std::string *)>;
  using SetValueCallback = std::function<int(std::string)>;

  inline static std::unique_ptr<CallbackXAttr> Create(
      const std::string &key, const GetValueCallback &get_callback,
      const SetValueCallback &set_callback, int mode) {
    return std::unique_ptr<CallbackXAttr>(
        new CallbackXAttr(key, get_callback, set_callback, mode));
  }

  ~CallbackXAttr() override = default;

  int SetValue(const char *value, size_t size) override;
  int GetValue(char *buffer, size_t max_size) const override;

  void ToHeader(std::string *header, std::string *value) const override;
  std::string ToString() const override;

 private:
  inline CallbackXAttr(const std::string &key,
                       const GetValueCallback &get_callback,
                       const SetValueCallback &set_callback, int mode)
      : XAttr(key, mode),
        get_callback_(get_callback),
        set_callback_(set_callback) {}

  const GetValueCallback get_callback_;
  const SetValueCallback set_callback_;
};
}  // namespace fs
}  // namespace s3

#endif
