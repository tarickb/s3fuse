/*
 * fs/static_xattr.cc
 * -------------------------------------------------------------------------
 * Static object extended attribute implementation.
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

#include "fs/static_xattr.h"

#include <errno.h>
#include <string.h>

#include <cctype>
#include <stdexcept>

#include "crypto/base64.h"
#include "crypto/encoder.h"
#include "crypto/hash.h"
#include "crypto/hex.h"
#include "crypto/md5.h"
#include "fs/metadata.h"

namespace s3 {
namespace fs {

namespace {
constexpr size_t MAX_STRING_SCAN_LEN = 128;

inline bool IsKeyValid(const std::string &key) {
  if (strncmp(key.c_str(), Metadata::RESERVED_PREFIX,
              strlen(Metadata::RESERVED_PREFIX)) == 0)
    return false;
  if (strncmp(key.c_str(), Metadata::XATTR_PREFIX,
              strlen(Metadata::XATTR_PREFIX)) == 0)
    return false;
  for (size_t i = 0; i < key.length(); i++)
    if (key[i] != '.' && key[i] != '-' && key[i] != '_' && !isdigit(key[i]) &&
        !islower(key[i]))
      return false;
  return true;
}

inline bool IsValueValid(const char *value, size_t size) {
  if (size > MAX_STRING_SCAN_LEN) return false;
  for (size_t i = 0; i < size; i++)
    if (value[i] < 32 ||
        value[i] == 127)  // see http://tools.ietf.org/html/rfc2616#section-2.2
      return false;
  return true;
}
}  // namespace

std::unique_ptr<StaticXAttr> StaticXAttr::FromHeader(
    const std::string &header_key, const std::string &header_value, int mode) {
  std::unique_ptr<StaticXAttr> ret;

  if (strncmp(header_key.c_str(), Metadata::XATTR_PREFIX,
              strlen(Metadata::XATTR_PREFIX)) == 0) {
    const size_t separator = header_value.find(' ');
    if (separator == std::string::npos)
      throw std::runtime_error("header string is malformed.");

    const auto dec_key = crypto::Encoder::Decode<crypto::Base64>(
        header_value.substr(0, separator));
    ret.reset(new StaticXAttr(reinterpret_cast<const char *>(&dec_key[0]), true,
                              true, mode));
    ret->value_ = crypto::Encoder::Decode<crypto::Base64>(
        header_value.substr(separator + 1));
  } else {
    // we know the value doesn't need encoding because it came to us as a valid
    // HTTP string.
    ret.reset(new StaticXAttr(header_key, false, false, mode));
    ret->value_.resize(header_value.size());
    memcpy(&ret->value_[0],
           reinterpret_cast<const uint8_t *>(header_value.c_str()),
           header_value.size());
  }

  if (ret->hide_on_empty_ && ret->value_.empty())
    ret->set_mode(ret->mode() & ~XM_VISIBLE);

  return ret;
}

std::unique_ptr<StaticXAttr> StaticXAttr::FromString(const std::string &key,
                                                     const std::string &value,
                                                     int mode) {
  auto ret = Create(key, mode);
  // terminating nulls are not stored
  ret->SetValue(value.c_str(), value.size());
  return ret;
}

std::unique_ptr<StaticXAttr> StaticXAttr::Create(const std::string &key,
                                                 int mode) {
  return std::unique_ptr<StaticXAttr>(
      new StaticXAttr(key, !IsKeyValid(key), true, mode));
}

int StaticXAttr::SetValue(const char *value, size_t size) {
  value_.resize(size);
  memcpy(&value_[0], reinterpret_cast<const uint8_t *>(value), size);
  encode_value_ = !IsValueValid(value, size);
  if (hide_on_empty_)
    set_mode(size ? (mode() | XM_VISIBLE) : (mode() & ~XM_VISIBLE));
  return 0;
}

int StaticXAttr::GetValue(char *buffer, size_t max_size) const {
  // so this is a little convoluted, but:
  //
  // if buffer == nullptr, return the size of the value
  // if buffer != nullptr and value_size > max_size, copy up to max_size and
  // return -ERANGE if buffer != nullptr and value_size <= max_size, copy up to
  // value_size and return value_size

  if (buffer == nullptr) return value_.size();
  const size_t size = (value_.size() > max_size) ? max_size : value_.size();
  memcpy(reinterpret_cast<uint8_t *>(buffer), &value_[0], size);
  return (size == value_.size()) ? size : -ERANGE;
}

void StaticXAttr::ToHeader(std::string *header, std::string *value) const {
  if (encode_key_ || encode_value_) {
    *header = std::string(Metadata::XATTR_PREFIX) +
              crypto::Hash::Compute<crypto::Md5, crypto::Hex>(key());
    *value = crypto::Encoder::Encode<crypto::Base64>(key()) + " " +
             crypto::Encoder::Encode<crypto::Base64>(value_);
  } else {
    *header = key();
    value->resize(value_.size());
    memcpy(&(*value)[0], &value_[0], value_.size());
  }
}

std::string StaticXAttr::ToString() const {
  std::string s;
  if (encode_value_)
    throw std::runtime_error("value cannot be represented as a string");
  s.assign(reinterpret_cast<const char *>(&value_[0]), value_.size());
  return s;
}

}  // namespace fs
}  // namespace s3
