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

#include <errno.h>
#include <string.h>

#include <stdexcept>

#include "base/logger.h"
#include "fs/callback_xattr.h"

namespace s3 {
namespace fs {

int callback_xattr::set_value(const char *value, size_t size) {
  std::string s;

  s.assign(value, size);

  return _set_fn(s);
}

int callback_xattr::get_value(char *buffer, size_t max_size) {
  std::string value;
  size_t size;
  int r;

  r = _get_fn(&value);

  if (r)
    return r;

  size = (value.size() > max_size) ? max_size : value.size();

  // see note in static_xattr::get_value() about buffer == NULL, etc.

  if (buffer == NULL)
    return value.size();

  memcpy(buffer, value.c_str(), size);

  return (size == value.size()) ? size : -ERANGE;
}

void callback_xattr::to_header(std::string *header, std::string *value) {
  throw std::runtime_error("cannot serialize callback xattr to header");
}

} // namespace fs
} // namespace s3
