#ifndef S3_CRYPTO_HEX_H
#define S3_CRYPTO_HEX_H

#include <stdint.h>

#include <string>
#include <vector>

namespace s3
{
  namespace crypto
  {
    class encoder;
    class hex_with_quotes;

    class hex
    {
    private:
      friend class encoder;
      friend class hex_with_quotes;

      static std::string encode(const uint8_t *input, size_t size);
      static void decode(const std::string &input, std::vector<uint8_t> *output);
    };
  }
}

#endif
