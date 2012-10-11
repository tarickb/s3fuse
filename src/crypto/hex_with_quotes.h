#ifndef S3_CRYPTO_HEX_WITH_QUOTES_H
#define S3_CRYPTO_HEX_WITH_QUOTES_H

#include <stdexcept>

#include "crypto/hex.h"

namespace s3
{
  namespace crypto
  {
    class hex_with_quotes
    {
    public:
      inline static std::string encode(const uint8_t *input, size_t size)
      {
        return std::string("\"") + hex::encode(input, size) + "\"";
      }

      inline static void decode(const std::string &input, std::vector<uint8_t> *output)
      {
        if (input[0] != '"' || input[input.size() - 1] != '"')
          throw std::runtime_error("hex input does not have surrounding quotes");

        return hex::decode(input.substr(0, input.size() - 2));
      }
    };
  }
}

#endif
