/*
 * crypto/pbkdf2_sha1.cc
 * -------------------------------------------------------------------------
 * PBKDF2 key derivation with SHA1 (implementation).
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

#include <openssl/evp.h>

#include <vector>

#include "crypto/buffer.h"
#include "crypto/pbkdf2_sha1.h"

namespace s3 {
namespace crypto {

buffer::ptr pbkdf2_sha1::derive(const std::string &password,
                                const std::string &salt, int rounds,
                                size_t key_len) {
  int r = 0;
  std::vector<uint8_t> key(key_len);

  r = PKCS5_PBKDF2_HMAC_SHA1(password.c_str(), password.size(),
                             reinterpret_cast<const uint8_t *>(salt.c_str()),
                             salt.size(), rounds, key_len, &key[0]);

  if (r != 1)
    throw std::runtime_error("failed to derive key");

  return buffer::from_vector(key);
}

} // namespace crypto
} // namespace s3
