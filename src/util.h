/*
 * util.h
 * -------------------------------------------------------------------------
 * Various utility methods, e.g., MD5 digests, Base64 encoding, etc.
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

#ifndef S3_UTIL_H
#define S3_UTIL_H

#include <stdint.h>
#include <stdio.h>

#include <string>
#include <vector>

namespace s3
{
  enum md5_output_type
  {
    MOT_BASE64,
    MOT_HEX,
    MOT_HEX_NO_QUOTE
  };

  class util
  {
  public:
    static std::string base64_encode(const uint8_t *input, size_t size);

    inline static std::string base64_encode(const char *input, size_t size)
    {
      return base64_encode(reinterpret_cast<const uint8_t *>(input), size);
    }

    inline static std::string base64_encode(const std::string &input)
    {
      return base64_encode(input.c_str(), input.size() + 1);
    }

    static void base64_decode(const std::string &input, std::vector<uint8_t> *output);

    static std::string sign(const std::string &key, const std::string &data);

    static std::string compute_md5(int fd, md5_output_type type = MOT_BASE64, ssize_t size = 0, off_t offset = 0);
    static std::string compute_md5(const uint8_t *input, size_t size, md5_output_type type = MOT_BASE64);

    inline static std::string compute_md5(const char *input, size_t size, md5_output_type type = MOT_BASE64)
    {
      return compute_md5(reinterpret_cast<const uint8_t *>(input), size, type);
    }

    inline static std::string compute_md5(const std::string &input, md5_output_type type = MOT_BASE64)
    {
      return compute_md5(input.c_str(), input.size() + 1, type);
    }

    static std::string url_encode(const std::string &url);
    static std::string hex_encode(const uint8_t *input, size_t size);

    static double get_current_time();

    static bool is_valid_md5(const std::string &md5);
  };
}

#endif
