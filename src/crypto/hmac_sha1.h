#ifndef S3_CRYPTO_HMAC_SHA1_H
#define S3_CRYPTO_HMAC_SHA1_H

#include <string>

namespace s3
{
  namespace crypto
  {
    class hmac_sha1
    {
    public:
      enum { MAC_LEN = 160 / 8 };

      static void sign(const uint8_t *key, size_t key_len, const uint8_t *data, size_t data_len, uint8_t *mac);

      inline static void sign(const std::string &key, const std::string &data, uint8_t *mac)
      {
        return sign(
          reinterpret_cast<const uint8_t *>(key.c_str()),
          key.size(),
          reinterpret_cast<const uint8_t *>(data.c_str()),
          data.size(),
          mac);
      }
    };
  }
}

#endif
