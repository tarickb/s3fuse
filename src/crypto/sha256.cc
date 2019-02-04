/*
 * crypto/sha256.cc
 * -------------------------------------------------------------------------
 * SHA256 hasher (implementation).
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

#include <openssl/sha.h>

#include "crypto/sha256.h"

namespace s3 {
  namespace crypto {

void sha256::compute(const uint8_t *input, size_t size, uint8_t *hash)
{
  SHA256(input, size, hash);
}

}  // namespace crypto
}  // namespace s3P
