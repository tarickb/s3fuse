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
