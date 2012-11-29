/*
 * crypto/md5.h
 * -------------------------------------------------------------------------
 * MD5 hasher.
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

#ifndef S3_CRYPTO_MD5_H
#define S3_CRYPTO_MD5_H

#include <stdint.h>

#include <string>

namespace s3
{
  namespace crypto
  {
    class hash;

    class md5
    {
    public:
      enum { HASH_LEN = 128 / 8 };

      inline static bool is_valid_quoted_hex_hash(const std::string &hash)
      {
        if (hash.size() != 2 * HASH_LEN + 2) // *2 for hex encoding, +2 for quotes
          return false;

        if (hash[0] != '"' || hash[hash.size() - 1] != '"')
          return false;

        for (size_t i = 1; i < hash.size() - 1; i++)
          if ((hash[i] < '0' || hash[i] > '9') && (hash[i] < 'a' || hash[i] > 'f') && (hash[i] < 'A' || hash[i] > 'F'))
            return false;

        return true;
      }

    private:
      friend class hash;

      static void compute(const uint8_t *input, size_t size, uint8_t *hash);
      static void compute(int fd, uint8_t *hash);
    };
  }
}

#endif
