/*
 * fs/callback_xattr.cc
 * -------------------------------------------------------------------------
 * Callback object extended attribute implementation.
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

#include "fs/callback_xattr.h"

#include <errno.h>
#include <string.h>

#include <stdexcept>

#include "base/logger.h"

namespace s3 {
namespace fs {

int CallbackXAttr::SetValue(const char *value, size_t size) {
  return set_callback_(std::string(value, size));
}

int CallbackXAttr::GetValue(char *buffer, size_t max_size) const {
  std::string value;
  const int r = get_callback_(&value);
  if (r) return r;
  // see note in static_xattr::get_value() about buffer == nullptr, etc.
  const size_t size = (value.size() > max_size) ? max_size : value.size();
  if (buffer == nullptr) return value.size();
  memcpy(buffer, value.c_str(), size);
  return (size == value.size()) ? size : -ERANGE;
}

void CallbackXAttr::ToHeader(std::string *header, std::string *value) const {
  throw std::runtime_error("cannot serialize callback xattr to header");
}

std::string CallbackXAttr::ToString() const {
  throw std::runtime_error("cannot cast callback xattr to string");
}

}  // namespace fs
}  // namespace s3
