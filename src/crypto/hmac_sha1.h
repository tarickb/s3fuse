/*
 * crypto/hmac_sha1.h
 * -------------------------------------------------------------------------
 * HMAC SHA1 message signer.
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

#ifndef S3_CRYPTO_HMAC_SHA1_H
#define S3_CRYPTO_HMAC_SHA1_H

#include <stdint.h>

#include <string>

namespace s3
{
  namespace crypto
  {
    class hmac_sha1
    {
    public:
      enum { MAC_LEN = 160 / 8 };

      static void sign(const uint8_t *key, size_t key_len, const uint8_t *data, size_t data_len, uint8_t *mac);

      inline static void sign(const std::string &key, const std::string &data, uint8_t *mac)
      {
        return sign(
          reinterpret_cast<const uint8_t *>(key.c_str()),
          key.size(),
          reinterpret_cast<const uint8_t *>(data.c_str()),
          data.size(),
          mac);
      }
    };
  }
}

#endif
