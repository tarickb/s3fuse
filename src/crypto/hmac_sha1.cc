#ifdef __APPLE__
  #include <CommonCrypto/CommonHMAC.h>
#else
  #include <openssl/hmac.h>
#endif

#include "crypto/hmac_sha1.h"

using s3::crypto::hmac_sha1;

void hmac_sha1::sign(const uint8_t *key, size_t key_len, const uint8_t *data, size_t data_len, uint8_t *mac)
{
  #ifdef __APPLE__
    CCHmac(
      kCCHmacAlgSHA1,
      reinterpret_cast<const void *>(key),
      key_len,
      reinterpret_cast<const void *>(data),
      data_len,
      mac);
  #else
    HMAC(
      EVP_sha1(), 
      reinterpret_cast<const void *>(key), 
      key_len,
      reinterpret_cast<const uint8_t *>(data), 
      data_len,
      mac, 
      NULL);
  #endif
}
