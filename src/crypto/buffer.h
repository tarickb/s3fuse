#ifndef S3_CRYPTO_BUFFER_H
#define S3_CRYPTO_BUFFER_H

#ifdef __APPLE__
  #include <Security/SecRandom.h>
#else
  #include <openssl/rand.h>
#endif

#include <stdexcept>
#include <string>
#include <vector>
#include <boost/smart_ptr.hpp>

#include "crypto/encoder.h"
#include "crypto/hex.h"

namespace s3
{
  namespace crypto
  {
    class buffer
    {
    public:
      typedef boost::shared_ptr<buffer> ptr;

      inline static ptr generate(size_t len)
      {
        ptr b(new buffer());

        b->_buf.resize(len);

        #ifdef __APPLE__
          if (SecRandomCopyBytes(kSecRandomDefault, len, &b->_buf[0]) != 0)
            throw std::runtime_error("failed to generate random key");
        #else
          if (RAND_bytes(&b->_buf[0], len) == 0)
            throw std::runtime_error("failed to generate random key");
        #endif

        return b;
      }

      inline static ptr from_string(const std::string &in)
      {
        ptr b(new buffer());

        encoder::decode<hex>(in, &b->_buf);

        return b;
      }

      inline const uint8_t * get() const { return &_buf[0]; }
      inline size_t size() const { return _buf.size(); }

      inline std::string to_string()
      {
        return encoder::encode<hex>(_buf);
      }

    private:
      inline buffer()
      {
      }

      std::vector<uint8_t> _buf;
    };
  }
}

#endif
