/*
 * crypto/aes_ctr_256.cc
 * -------------------------------------------------------------------------
 * AES in counter mode (implementation).
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

#include <string.h>

#include "crypto/aes_ctr_256.h"
#include "crypto/symmetric_key.h"

using std::runtime_error;

using s3::crypto::aes_ctr_256;
using s3::crypto::symmetric_key;

void aes_ctr_256::crypt(const symmetric_key::ptr &key, uint64_t starting_block, const uint8_t *in, size_t size, uint8_t *out)
{
  uint8_t iv[BLOCK_LEN];

  if (key->get_iv()->size() != IV_LEN)
    throw runtime_error("iv length is not valid for aes_ctr_256");

  starting_block = __builtin_bswap64(starting_block);

  memcpy(iv, key->get_iv()->get(), IV_LEN);
  memcpy(iv + IV_LEN, &starting_block, sizeof(starting_block));

  #ifdef __APPLE__
    CCCryptorRef cryptor;
    CCCryptorStatus r;

    r = CCCryptorCreateWithMode(
      kCCEncrypt, // use for both encryption and decryption because ctr is symmetric
      kCCModeCTR,
      kCCAlgorithmAES128,
      ccNoPadding,
      iv,
      key->get_key()->get(),
      key->get_key()->size(),
      NULL,
      0,
      0,
      kCCModeOptionCTR_BE,
      &cryptor);

    if (r != kCCSuccess)
      throw runtime_error("call to CCCryptorCreateWithMode() failed in aes_ctr_256");

    r = CCCryptorUpdate(
      cryptor,
      in,
      size,
      out,
      size,
      NULL);

    CCCryptorRelease(cryptor);

    if (r != kCCSuccess)
      throw runtime_error("CCCryptorUpdate() failed in aes_ctr_256");
  #else
    AES_KEY aes_key;
    uint8_t ecount_buf[BLOCK_LEN];
    unsigned int num = 0;

    memset(ecount_buf, 0, sizeof(ecount_buf));

    if (AES_set_encrypt_key(key->get_key()->get(), key->get_key()->size() * 8 /* in bits */, &aes_key) != 0)
      throw runtime_error("failed to set encryption key for aes_ctr_256");

    AES_ctr128_encrypt(in, out, size, &aes_key, iv, ecount_buf, &num);
  #endif
}
