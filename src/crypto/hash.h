/*
 * crypto/hash.h
 * -------------------------------------------------------------------------
 * Templated interface to hash classes.
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

#ifndef S3_CRYPTO_HASH_H
#define S3_CRYPTO_HASH_H

#include <stdint.h>

#include <string>
#include <vector>

#include "crypto/encoder.h"

namespace s3
{
  namespace crypto
  {
    class hash
    {
    public:
      template <class hash_type>
      inline static void compute(const uint8_t *input, size_t size, uint8_t *hash)
      {
        hash_type::compute(input, size, hash);
      }

      template <class hash_type>
      inline static void compute(const char *input, size_t size, uint8_t *hash)
      {
        hash_type::compute(reinterpret_cast<const uint8_t *>(input), size, hash);
      }

      template <class hash_type>
      inline static void compute(const std::string &input, uint8_t *hash)
      {
        compute<hash_type>(input.c_str(), input.size() + 1, hash);
      }

      template <class hash_type>
      inline static void compute(const std::vector<uint8_t> &input, uint8_t *hash)
      {
        hash_type::compute(&input[0], input.size(), hash);
      }

      template <class hash_type>
      inline static void compute(const std::vector<char> &input, uint8_t *hash)
      {
        compute<hash_type>(&input[0], input.size(), hash);
      }

      template <class hash_type>
      inline static void compute(int fd, uint8_t *hash)
      {
        hash_type::compute(fd, hash);
      }

      template <class hash_type, class encoder_type>
      inline static std::string compute(const uint8_t *input, size_t size)
      {
        uint8_t hash[hash_type::HASH_LEN];

        hash_type::compute(input, size, hash);

        return encoder::encode<encoder_type>(hash, hash_type::HASH_LEN);
      }

      template <class hash_type, class encoder_type>
      inline static std::string compute(const std::string &input)
      {
        uint8_t hash[hash_type::HASH_LEN];

        compute<hash_type>(input, hash);

        return encoder::encode<encoder_type>(hash, hash_type::HASH_LEN);
      }

      template <class hash_type, class encoder_type>
      inline static std::string compute(const char *input, size_t size)
      {
        uint8_t hash[hash_type::HASH_LEN];

        compute<hash_type>(input, size, hash);

        return encoder::encode<encoder_type>(hash, hash_type::HASH_LEN);
      }

      template <class hash_type, class encoder_type>
      inline static std::string compute(const std::vector<uint8_t> &input)
      {
        uint8_t hash[hash_type::HASH_LEN];

        compute<hash_type>(input, hash);

        return encoder::encode<encoder_type>(hash, hash_type::HASH_LEN);
      }

      template <class hash_type, class encoder_type>
      inline static std::string compute(const std::vector<char> &input)
      {
        uint8_t hash[hash_type::HASH_LEN];

        compute<hash_type>(input, hash);

        return encoder::encode<encoder_type>(hash, hash_type::HASH_LEN);
      }

      template <class hash_type, class encoder_type>
      inline static std::string compute(int fd)
      {
        uint8_t hash[hash_type::HASH_LEN];

        compute<hash_type>(fd, hash);

        return encoder::encode<encoder_type>(hash, hash_type::HASH_LEN);
      }
    };
  }
}

#endif
