#ifdef __APPLE__
  #include <CommonCrypto/CommonKeyDerivation.h>
#else
  #include <openssl/evp.h>
#endif

#include <vector>

#include "crypto/buffer.h"
#include "crypto/pbkdf2_sha1.h"

using std::runtime_error;
using std::string;
using std::vector;

using s3::crypto::buffer;
using s3::crypto::pbkdf2_sha1;

buffer::ptr pbkdf2_sha1::derive(const string &password, const string &salt, int rounds, size_t key_len)
{
  int r = 0;
  vector<uint8_t> key(key_len);

  #ifdef __APPLE__
    r = CCKeyDerivationPBKDF(
      kCCPBKDF2,
      password.c_str(),
      password.size(),
      reinterpret_cast<const uint8_t *>(salt.c_str()),
      salt.size(),
      kCCPRFHmacAlgSHA1,
      rounds,
      &key[0],
      key_len);

    if (r != kCCSuccess)
      throw runtime_error("failed to derive key");
  #else
    r = PKCS5_PBKDF2_HMAC_SHA1(
      password.c_str(),
      password.size(),
      reinterpret_cast<const uint8_t *>(salt.c_str()),
      salt.size(),
      rounds,
      key_len,
      &key[0]);

    if (r != 1)
      throw runtime_error("failed to derive key");
  #endif

  return buffer::from_bytes(key);
}
