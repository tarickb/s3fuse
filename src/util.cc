/*
 * util.cc
 * -------------------------------------------------------------------------
 * MD5 digests, various encodings.
 * -------------------------------------------------------------------------
 *
 * Copyright (c) 2011, Tarick Bedeir.
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

#include "util.h"

#ifdef __APPLE__
  #include <util_macos.inc.cc>
#else
  #include <util_gnu.inc.cc>
#endif

string util::hex_encode(const uint8_t *input, size_t size)
{
  const char *hex = "0123456789abcdef";
  string ret;

  ret.resize(size * 2);

  for (size_t i = 0; i < size; i++) {
    ret[2 * i + 0] = hex[static_cast<uint8_t>(input[i]) / 16];
    ret[2 * i + 1] = hex[static_cast<uint8_t>(input[i]) % 16];
  }

  return ret;
}

string util::url_encode(const string &url)
{
  const char *hex = "0123456789ABCDEF";
  string ret;

  ret.reserve(url.length());

  for (size_t i = 0; i < url.length(); i++) {
    if (url[i] == '/' || url[i] == '.' || url[i] == '-' || url[i] == '*' || url[i] == '_' || isalnum(url[i]))
      ret += url[i];
    else {
      // allow spaces to be encoded as "%20" rather than "+" because Google
      // Storage doesn't decode the same way AWS does

      ret += '%';
      ret += hex[static_cast<uint8_t>(url[i]) / 16];
      ret += hex[static_cast<uint8_t>(url[i]) % 16];
    }
  }

  return ret;
}

double util::get_current_time()
{
  timeval t;

  gettimeofday(&t, NULL);

  return double(t.tv_sec) + double(t.tv_usec) / 1.0e6;
}

bool util::is_valid_md5(const string &md5)
{
  // yes, it's a rather rudimentary check (32 for the MD5, 2 for the quotes)
  return (md5.size() == 34 && md5[0] == '"' && md5[md5.size() - 1] == '"');
}
