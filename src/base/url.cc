/*
 * base/url.cc
 * -------------------------------------------------------------------------
 * URL-related functions.
 * -------------------------------------------------------------------------
 *
 * Copyright (c) 2019, Tarick Bedeir.
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

#include "base/url.h"

#include <cstdint>
#include <string>

namespace s3 {
namespace base {

std::string Url::Encode(const std::string &url) {
  constexpr char HEX[] = "0123456789ABCDEF";

  std::string ret;
  ret.reserve(url.length());

  for (size_t i = 0; i < url.length(); i++) {
    if (url[i] == '/' || url[i] == '.' || url[i] == '-' || url[i] == '*' ||
        url[i] == '_' || isalnum(url[i])) {
      ret += url[i];
    } else {
      // allow spaces to be encoded as "%20" rather than "+" because Google
      // Storage doesn't decode the same way AWS does
      ret += '%';
      ret += HEX[static_cast<uint8_t>(url[i]) / 16];
      ret += HEX[static_cast<uint8_t>(url[i]) % 16];
    }
  }

  return ret;
}

}  // namespace base
}  // namespace s3
