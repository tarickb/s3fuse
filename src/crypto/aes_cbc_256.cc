#include "crypto/aes_cbc_256.h"
#include "crypto/symmetric_key.h"

using std::runtime_error;
using std::vector;

using s3::crypto::aes_cbc_256;
using s3::crypto::symmetric_key;

void aes_cbc_256::crypt(crypt_mode mode, const symmetric_key::ptr &key, const uint8_t *in, size_t size, vector<uint8_t> *out)
{
  if (key->get_iv()->size() != IV_LEN)
    throw runtime_error("iv length is not valid for aes_cbc_256");

  #ifdef __APPLE__
    CCCryptorStatus r;
    size_t written = 0;

    if (mode == M_ENCRYPT) {
      // allow for padding
      out->resize(size + BLOCK_LEN);
    } else {
      if (size % BLOCK_LEN)
        throw runtime_error("input size must be a multiple of BLOCK_LEN when decrypting with aes_cbc_256");

      out->resize(size);
    }

    r = CCCrypt(
      (mode == M_ENCRYPT ? kCCEncrypt : kCCDecrypt),
      kCCAlgorithmAES128,
      kCCOptionPKCS7Padding,
      key->get_key()->get(),
      key->get_key()->size(),
      key->get_iv()->get(),
      in,
      size,
      &(*out)[0],
      out->size(),
      &written);

    if (r != kCCSuccess)
      throw runtime_error("CCCrypt() failed in aes_cbc_256");

    out->resize(written);
  #else
    #error add openssl version of this
  #endif
}
