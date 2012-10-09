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

#include <stdexcept>
#include <string>
#include <vector>

namespace s3
{
  enum encoding
  {
    E_BASE64,
    E_HEX_WITH_QUOTES,
    E_HEX
  };

  class util
  {
  public:
    static std::string sign(const std::string &key, const std::string &data);

    static void compute_md5(const uint8_t *input, size_t size, std::vector<uint8_t> *output);
    static void compute_md5(int fd, std::vector<uint8_t> *output);

    static std::string url_encode(const std::string &url);

    static double get_current_time();

    static bool is_valid_md5(const std::string &md5);

    inline static std::string encode(const uint8_t *input, size_t size, encoding type)
    {
      if (type == E_BASE64)
        return base64_encode(input, size);
      else if (type == E_HEX_WITH_QUOTES)
        return std::string("\"") + hex_encode(input, size) + "\"";
      else if (type == E_HEX)
        return hex_encode(input, size);
      else
        throw std::runtime_error("unknown encoding type in util::encode()");
    }

    inline static std::string encode(const std::string &input, encoding type)
    {
      return encode(reinterpret_cast<const uint8_t *>(input.c_str()), input.size() + 1, type);
    }

    inline static std::string encode(const std::vector<uint8_t> &input, encoding type)
    {
      return encode(&input[0], input.size(), type);
    }

    inline static void decode(const std::string &input, encoding type, std::vector<uint8_t> *output)
    {
      if (type == E_BASE64)
        base64_decode(input, output);
      else if (type == E_HEX)
        hex_decode(input, output);
      else if (type == E_HEX_WITH_QUOTES) {
        if (input[0] != '\"' || input[input.size() - 1] != '\"')
          throw std::runtime_error("malformed hex-with-quotes string");

        hex_decode(input.substr(1, input.size() - 2), output);
      } else
        throw std::runtime_error("unknown encoding type in util::decode()");
    }

    inline static std::string compute_md5(const uint8_t *input, size_t size, encoding type)
    {
      std::vector<uint8_t> buf;

      compute_md5(input, size, &buf);

      return encode(&buf[0], buf.size(), type);
    }

    inline static std::string compute_md5(const std::string &input, encoding type)
    {
      return compute_md5(reinterpret_cast<const uint8_t *>(input.c_str()), input.size() + 1, type);
    }

    inline static std::string compute_md5(int fd, encoding type)
    {
      std::vector<uint8_t> buf;

      compute_md5(fd, &buf);

      return encode(&buf[0], buf.size(), type);
    }

    inline static void compute_md5(const std::vector<uint8_t> &input, std::vector<uint8_t> *output)
    {
      compute_md5(&input[0], input.size(), output);
    }

    inline static void compute_md5(const std::vector<char> &input, std::vector<uint8_t> *output)
    {
      compute_md5(reinterpret_cast<const uint8_t *>(&input[0]), input.size(), output);
    }

    inline static std::string compute_md5(const std::vector<uint8_t> &input, encoding type)
    {
      return compute_md5(&input[0], input.size(), type);
    }

    inline static std::string compute_md5(const std::vector<char> &input, encoding type)
    {
      return compute_md5(reinterpret_cast<const uint8_t *>(&input[0]), input.size(), type);
    }

  private:
    static std::string base64_encode(const uint8_t *input, size_t size);
    static void base64_decode(const std::string &input, std::vector<uint8_t> *output);

    static std::string hex_encode(const uint8_t *input, size_t size);
    static void hex_decode(const std::string &input, std::vector<uint8_t> *output);
  };
}

#endif
