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

#ifdef __APPLE__
  #include <CommonCrypto/CommonCrypto.h>
#else
  #include <openssl/evp.h>
#endif

#include <vector>

#include "crypto/buffer.h"
#include "crypto/pbkdf2_sha1.h"

using std::runtime_error;
using std::string;
using std::vector;

using s3::crypto::buffer;
using s3::crypto::pbkdf2_sha1;

buffer::ptr pbkdf2_sha1::derive(const string &password, const string &salt, int rounds, size_t key_len)
{
  int r = 0;
  vector<uint8_t> key(key_len);

  #ifdef __APPLE__
    r = CCKeyDerivationPBKDF(
      kCCPBKDF2,
      password.c_str(),
      password.size(),
      reinterpret_cast<const uint8_t *>(salt.c_str()),
      salt.size(),
      kCCPRFHmacAlgSHA1,
      rounds,
      &key[0],
      key_len);

    if (r != kCCSuccess)
      throw runtime_error("failed to derive key");
  #else
    r = PKCS5_PBKDF2_HMAC_SHA1(
      password.c_str(),
      password.size(),
      reinterpret_cast<const uint8_t *>(salt.c_str()),
      salt.size(),
      rounds,
      key_len,
      &key[0]);

    if (r != 1)
      throw runtime_error("failed to derive key");
  #endif

  return buffer::from_bytes(key);
}
