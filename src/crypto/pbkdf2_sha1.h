/*
 * crypto/pbkdf2_sha1.h
 * -------------------------------------------------------------------------
 * PBKDF2 key derivation with SHA1.
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

#ifndef S3_CRYPTO_PBKDF2_SHA1_H
#define S3_CRYPTO_PBKDF2_SHA1_H

#include <string>
#include <boost/smart_ptr.hpp>

namespace s3
{
  namespace crypto
  {
    class buffer;

    class pbkdf2_sha1
    {
    public:
      template <class cipher_type>
      inline static boost::shared_ptr<buffer> derive(const std::string &password, const std::string &salt, int rounds)
      {
        return derive(password, salt, rounds, cipher_type::DEFAULT_KEY_LEN);
      }

      static boost::shared_ptr<buffer> derive(const std::string &password, const std::string &salt, int rounds, size_t key_len);
    };
  }
}

#endif
