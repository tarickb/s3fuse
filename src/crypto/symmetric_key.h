/*
 * crypto/symmetric_key.h
 * -------------------------------------------------------------------------
 * Contains a symmetric key and an initialization vector, and allows for
 * serialization and deserialization.
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

#ifndef S3_CRYPTO_SYMMETRIC_KEY_H
#define S3_CRYPTO_SYMMETRIC_KEY_H

#include <string>
#include <boost/smart_ptr.hpp>

#include "crypto/buffer.h"

namespace s3
{
  namespace crypto
  {
    class symmetric_key
    {
    public:
      typedef boost::shared_ptr<symmetric_key> ptr;

      template <class cipher_type>
      inline static ptr generate()
      {
        return create(buffer::generate(cipher_type::DEFAULT_KEY_LEN), buffer::generate(cipher_type::IV_LEN));
      }

      template <class cipher_type>
      inline static ptr generate(size_t key_len)
      {
        return create(buffer::generate(key_len), buffer::generate(cipher_type::IV_LEN));
      }

      template <class cipher_type>
      inline static ptr generate(const buffer::ptr &key)
      {
        return create(key, buffer::generate(cipher_type::IV_LEN));
      }

      inline static ptr from_string(const std::string &str)
      {
        size_t pos;

        pos = str.find(':');

        if (pos == std::string::npos)
          throw std::runtime_error("malformed symmetric key string");

        return create(buffer::from_string(str.substr(0, pos)), buffer::from_string(str.substr(pos + 1)));
      }

      inline static ptr create(const buffer::ptr &key, const buffer::ptr &iv)
      {
        ptr sk(new symmetric_key());

        sk->_key = key;
        sk->_iv = iv;

        return sk;
      }

      inline const buffer::ptr & get_key() { return _key; }
      inline const buffer::ptr & get_iv() { return _iv; }

      inline std::string to_string()
      {
        return _key->to_string() + ":" + _iv->to_string();
      }

    private:
      inline symmetric_key()
      {
      }

      buffer::ptr _key, _iv;
    };
  }
}

#endif
