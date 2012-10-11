#ifdef __APPLE__
  #include <CommonCrypto/CommonDigest.h>
#else
  #include <openssl/sha.h>
#endif

#include "crypto/sha256.h"

using s3::crypto::sha256;

void sha256::compute(const uint8_t *input, size_t size, uint8_t *hash)
{
  #ifdef __APPLE__
    CC_SHA256(input, size, hash);
  #else
    SHA256(input, size, hash);
  #endif
}
