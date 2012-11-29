/*
 * crypto/sha256.h
 * -------------------------------------------------------------------------
 * SHA256 hasher.
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

#ifndef S3_CRYPTO_SHA256_H
#define S3_CRYPTO_SHA256_H

#include <stddef.h>
#include <stdint.h>

namespace s3
{
  namespace crypto
  {
    class hash;

    class sha256
    {
    public:
      enum { HASH_LEN = 256 / 8 };

    private:
      friend class hash;

      static void compute(const uint8_t *data, size_t size, uint8_t *hash);
    };
  }
}

#endif
