/*
 * crypto/encoder.h
 * -------------------------------------------------------------------------
 * Templated interface to encoder classes.
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

#ifndef S3_CRYPTO_ENCODER_H
#define S3_CRYPTO_ENCODER_H

#include <stdint.h>

#include <string>
#include <vector>

namespace s3
{
  namespace crypto
  {
    class encoder
    {
    public:
      template <class encoder_type>
      inline static std::string encode(const uint8_t *input, size_t size)
      {
        return encoder_type::encode(input, size);
      }

      template <class encoder_type>
      inline static std::string encode(const char *input, size_t size)
      {
        return encoder_type::encode(reinterpret_cast<const uint8_t *>(input), size);
      }

      template <class encoder_type>
      inline static std::string encode(const std::string &input)
      {
        return encoder_type::encode(reinterpret_cast<const uint8_t *>(input.c_str()), input.size() + 1);
      }

      template <class encoder_type>
      inline static std::string encode(const std::vector<uint8_t> &input)
      {
        return encoder_type::encode(&input[0], input.size());
      }

      template <class encoder_type>
      inline static void decode(const std::string &input, std::vector<uint8_t> *output)
      {
        encoder_type::decode(input, output);
      }
    };
  }
}

#endif
