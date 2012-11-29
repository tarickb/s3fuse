/*
 * crypto/aes_ctr_256.h
 * -------------------------------------------------------------------------
 * AES in counter mode with a default key length of 256 bits.
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

#ifndef S3_CRYPTO_AES_CTR_256_H
#define S3_CRYPTO_AES_CTR_256_H

#include <stdint.h>

#ifdef __APPLE__
  #include <CommonCrypto/CommonCryptor.h>
#else
  #include <openssl/aes.h>
#endif

#include <stdexcept>
#include <boost/smart_ptr.hpp>

namespace s3
{
  namespace crypto
  {
    class symmetric_key;

    // TODO: move starting block parameter (or offset) to encrypt method so that only one cipher is created.. maybe?
    class aes_ctr_256
    {
    public:
      typedef boost::shared_ptr<aes_ctr_256> ptr;

      #ifdef __APPLE__
        enum { BLOCK_LEN = kCCBlockSizeAES128 };
      #else
        enum { BLOCK_LEN = AES_BLOCK_SIZE };
      #endif

      enum { IV_LEN = BLOCK_LEN / 2 };
      enum { DEFAULT_KEY_LEN = 32 }; // 256 bits

      inline static ptr create(const boost::shared_ptr<symmetric_key> &key)
      {
        return create_with_starting_block(key, 0);
      }

      inline static ptr create_with_byte_offset(const boost::shared_ptr<symmetric_key> &key, uint64_t offset)
      {
        if (offset % BLOCK_LEN)
          throw std::runtime_error("offset must be a multiple of BLOCK_LEN");

        return create_with_starting_block(key, offset / BLOCK_LEN);
      }

      inline static ptr create_with_starting_block(const boost::shared_ptr<symmetric_key> &key, uint64_t starting_block)
      {
        return ptr(new aes_ctr_256(key, starting_block));
      }

      ~aes_ctr_256();

      inline void encrypt(const uint8_t *in, size_t size, uint8_t *out)
      {
        #ifdef __APPLE__
          CCCryptorStatus r;

          // TODO: find some way to force block boundary

          r = CCCryptorUpdate(
            _cryptor,
            in,
            size,
            out,
            size,
            NULL);

          if (r != kCCSuccess)
            throw std::runtime_error("CCCryptorUpdate() failed in aes_ctr_256");
        #else
          // by always setting _num = 0 before encrypting we enforce our 
          // requirement that all encryption be started on block boundaries

          _num = 0;
          AES_ctr128_encrypt(in, out, size, &_key, _iv, _ecount_buf, &_num);
        #endif
      }

      inline void decrypt(const uint8_t *in, size_t size, uint8_t *out)
      {
        encrypt(in, size, out);
      }

    private:
      aes_ctr_256(const boost::shared_ptr<symmetric_key> &key, uint64_t starting_block);

      #ifdef __APPLE__
        CCCryptorRef _cryptor;
      #else
        AES_KEY _key;
        uint8_t _iv[BLOCK_LEN];
        uint8_t _ecount_buf[BLOCK_LEN];
        unsigned int _num;
      #endif
    };
  }
}

#endif
