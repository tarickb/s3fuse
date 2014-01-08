/*
 * crypto/aes_cbc_256.cc
 * -------------------------------------------------------------------------
 * AES in cipher block chaining mode (implementation).
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

#include "crypto/aes_cbc_256.h"
#include "crypto/symmetric_key.h"

using std::runtime_error;
using std::vector;

using s3::crypto::aes_cbc_256;
using s3::crypto::symmetric_key;

void aes_cbc_256::crypt(int mode, const symmetric_key::ptr &key, const uint8_t *in, size_t size, vector<uint8_t> *out)
{
  if (key->get_iv()->size() != IV_LEN)
    throw runtime_error("iv length is not valid for aes_cbc_256");

  if (mode & M_ENCRYPT) {
    // allow for padding
    out->resize(size + BLOCK_LEN);
  } else {
    if (size % BLOCK_LEN)
      throw runtime_error("input size must be a multiple of BLOCK_LEN when decrypting with aes_cbc_256");

    out->resize(size);
  }

  EVP_CIPHER_CTX ctx;
  const EVP_CIPHER *cipher = NULL;

  switch (key->get_key()->size()) {
    case (128 / 8):
      cipher = EVP_aes_128_cbc();
      break;

    case (192 / 8):
      cipher = EVP_aes_192_cbc();
      break;

    case (256 / 8):
      cipher = EVP_aes_256_cbc();
      break;
  }

  if (!cipher)
    throw runtime_error("invalid key length for aes_cbc_256");
  
  EVP_CIPHER_CTX_init(&ctx);

  try {
    int updated = 0, finalized = 0;

    if (mode & M_ENCRYPT) {
      if (EVP_EncryptInit_ex(&ctx, cipher, NULL, key->get_key()->get(), key->get_iv()->get()) == 0)
        throw runtime_error("EVP_EncryptInit_ex() failed in aes_cbc_256");

      if (mode & M_NO_PAD)
        EVP_CIPHER_CTX_set_padding(&ctx, 0);

      if (!EVP_EncryptUpdate(&ctx, &(*out)[0], &updated, in, size))
        throw runtime_error("EVP_EncryptUpdate() failed in aes_cbc_256");

      if (!EVP_EncryptFinal_ex(&ctx, &(*out)[updated], &finalized))
        throw runtime_error("EVP_EncryptFinal_ex() failed in aes_cbc_256");
    } else {
      if (EVP_DecryptInit_ex(&ctx, cipher, NULL, key->get_key()->get(), key->get_iv()->get()) == 0)
        throw runtime_error("EVP_DecryptInit_ex() failed in aes_cbc_256");

      if (mode & M_NO_PAD)
        EVP_CIPHER_CTX_set_padding(&ctx, 0);

      if (!EVP_DecryptUpdate(&ctx, &(*out)[0], &updated, in, size))
        throw runtime_error("EVP_DecryptUpdate() failed in aes_cbc_256");

      if (!EVP_DecryptFinal_ex(&ctx, &(*out)[updated], &finalized))
        throw runtime_error("EVP_DecryptFinal_ex() failed in aes_cbc_256");
    }

    out->resize(updated + finalized);

  } catch (...) {
    EVP_CIPHER_CTX_cleanup(&ctx);
    throw;
  }

  EVP_CIPHER_CTX_cleanup(&ctx);
}
