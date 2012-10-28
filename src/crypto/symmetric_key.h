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
