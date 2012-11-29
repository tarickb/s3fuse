/*
 * crypto/base64.h
 * -------------------------------------------------------------------------
 * Base64 encoding/decoding.
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

#ifndef S3_CRYPTO_BASE64_H
#define S3_CRYPTO_BASE64_H

#include <stdint.h>

#include <string>
#include <vector>

namespace s3
{
  namespace crypto
  {
    class encoder;

    class base64
    {
    private:
      friend class encoder;

      static std::string encode(const uint8_t *input, size_t size);
      static void decode(const std::string &input, std::vector<uint8_t> *output);
    };
  }
}

#endif
