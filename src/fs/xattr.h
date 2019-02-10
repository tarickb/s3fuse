/*
 * fs/xattr.h
 * -------------------------------------------------------------------------
 * Base class for object extended attributes.
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

#ifndef S3_FS_XATTR_H
#define S3_FS_XATTR_H

#include <string>

namespace s3 {
namespace fs {
class XAttr {
 public:
  enum AccessMode {
    XM_DEFAULT = 0x00,
    XM_WRITABLE = 0x01,
    XM_SERIALIZABLE = 0x02,
    XM_VISIBLE = 0x04,
    XM_REMOVABLE = 0x08,
    XM_COMMIT_REQUIRED = 0x10
  };

  virtual ~XAttr() = default;

  inline std::string key() const { return key_; }

  inline bool is_writable() const { return mode_ & XM_WRITABLE; }
  inline bool is_serializable() const { return mode_ & XM_SERIALIZABLE; }
  inline bool is_visible() const { return mode_ & XM_VISIBLE; }
  inline bool is_removable() const { return mode_ & XM_REMOVABLE; }
  inline bool is_commit_required() const { return mode_ & XM_COMMIT_REQUIRED; }

  inline int mode() const { return mode_; }
  inline void set_mode(int mode) { mode_ = mode; }

  virtual int SetValue(const char *value, size_t size) = 0;
  virtual int GetValue(char *buffer, size_t max_size) const = 0;

  virtual void ToHeader(std::string *header, std::string *value) const = 0;
  virtual std::string ToString() const = 0;

 protected:
  inline XAttr(const std::string &key, int mode) : key_(key), mode_(mode) {}

 private:
  const std::string key_;
  int mode_;
};
}  // namespace fs
}  // namespace s3

#endif
