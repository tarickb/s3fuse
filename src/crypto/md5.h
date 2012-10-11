#ifndef S3_CRYPTO_MD5_H
#define S3_CRYPTO_MD5_H

#include <string>

namespace s3
{
  namespace crypto
  {
    class md5
    {
    public:
      enum { HASH_LEN = 128 / 8 };

      inline static bool is_valid_quoted_hex_hash(const std::string &hash)
      {
        if (hash.size() != HASH_LEN + 2) // +2 for quotes
          return false;

        if (hash[0] != '"' || hash[hash.size() - 1] != '"')
          return false;

        for (size_t i = 1; i < hash.size() - 1; i++)
          if ((hash[i] < '0' || hash[i] > '9') && (hash[i] < 'a' || hash[i] > 'f') && (hash[i] < 'A' || hash[i] > 'F'))
            return false;

        return true;
      }

      static void compute(const uint8_t *input, size_t size, uint8_t *hash);
      static void compute(int fd, uint8_t *hash);
    };
  }
}

#endif
