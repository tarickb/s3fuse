/*
 * crypto/hex_with_quotes.h
 * -------------------------------------------------------------------------
 * Hex encoder where input/output is enclosed by double quotes.
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

#ifndef S3_CRYPTO_HEX_WITH_QUOTES_H
#define S3_CRYPTO_HEX_WITH_QUOTES_H

#include <stdexcept>

#include "crypto/hex.h"

namespace s3
{
  namespace crypto
  {
    class encoder;

    class hex_with_quotes
    {
    private:
      friend class encoder;

      inline static std::string encode(const uint8_t *input, size_t size)
      {
        return std::string("\"") + hex::encode(input, size) + "\"";
      }

      inline static void decode(const std::string &input, std::vector<uint8_t> *output)
      {
        if (input[0] != '"' || input[input.size() - 1] != '"')
          throw std::runtime_error("hex input does not have surrounding quotes");

        return hex::decode(input.substr(1, input.size() - 2), output);
      }
    };
  }
}

#endif
