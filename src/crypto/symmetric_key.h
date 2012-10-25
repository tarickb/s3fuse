#ifndef S3_CRYPTO_SYMMETRIC_KEY_H
#define S3_CRYPTO_SYMMETRIC_KEY_H

#include <stdint.h>

#include <string>
#include <vector>
#include <boost/smart_ptr.hpp>

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
        ptr cs(new symmetric_key());

        cs->generate(cipher_type::DEFAULT_KEY_LEN, cipher_type::IV_LEN);

        return cs;
      }

      template <class cipher_type>
      inline static ptr generate(size_t key_len)
      {
        ptr cs(new symmetric_key());

        cs->generate(key_len, cipher_type::IV_LEN);

        return cs;
      }

      static ptr deserialize(const std::string &state);

      inline const uint8_t * get_key() const { return &_key[0]; }
      inline const uint8_t * get_iv() const { return &_iv[0]; }

      inline const size_t get_key_len() const { return _key.size(); }
      inline const size_t get_iv_len() const { return _iv.size(); }

      std::string serialize();

    private:
      inline symmetric_key()
      {
      }

      void generate(size_t key_len, size_t iv_len);

      std::vector<uint8_t> _key, _iv;
    };
  }
}

#endif
