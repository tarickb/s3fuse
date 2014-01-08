/*
 * crypto/hmac_sha1.cc
 * -------------------------------------------------------------------------
 * HMAC SHA1 message signing (implementation).
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

#include <openssl/hmac.h>

#include "crypto/hmac_sha1.h"

using s3::crypto::hmac_sha1;

void hmac_sha1::sign(const uint8_t *key, size_t key_len, const uint8_t *data, size_t data_len, uint8_t *mac)
{
  HMAC(
    EVP_sha1(), 
    reinterpret_cast<const void *>(key), 
    key_len,
    reinterpret_cast<const uint8_t *>(data), 
    data_len,
    mac, 
    NULL);
}
