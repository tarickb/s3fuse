#ifndef S3_CRYPTO_SHA256_H
#define S3_CRYPTO_SHA256_H

#include <stddef.h>
#include <stdint.h>

namespace s3
{
  namespace crypto
  {
    class hash;

    class sha256
    {
    public:
      enum { HASH_LEN = 256 / 8 };

    private:
      friend class hash;

      static void compute(const uint8_t *data, size_t size, uint8_t *hash);
    };
  }
}

#endif
