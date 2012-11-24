#ifndef __APPLE__
  #include <openssl/evp.h>
#endif

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

  if (mode == M_ENCRYPT) {
    // allow for padding
    out->resize(size + BLOCK_LEN);
  } else {
    if (size % BLOCK_LEN)
      throw runtime_error("input size must be a multiple of BLOCK_LEN when decrypting with aes_cbc_256");

    out->resize(size);
  }

  #ifdef __APPLE__
    CCCryptorStatus r;
    size_t written = 0;

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
    EVP_CIPHER_CTX ctx;
    const EVP_CIPHER *cipher = NULL;

    switch (key->get_key()->size()) {
      case (128 / 8):
        cipher = EVP_aes_128_cbc();
        break;

      case (192 / 8):
        cipher = EVP_aes_192_cbc();
        break;

      case (256 / 8):
        cipher = EVP_aes_256_cbc();
        break;
    }

    if (!cipher)
      throw runtime_error("invalid key length for aes_cbc_256");
    
    EVP_CIPHER_CTX_init(&ctx);

    try {
      int updated = 0, finalized = 0;

      if (mode == M_ENCRYPT) {
        if (EVP_EncryptInit_ex(&ctx, cipher, NULL, key->get_key()->get(), key->get_iv()->get()) == 0)
          throw runtime_error("EVP_EncryptInit_ex() failed in aes_cbc_256");

        if (!EVP_EncryptUpdate(&ctx, &(*out)[0], &updated, in, size))
          throw runtime_error("EVP_EncryptUpdate() failed in aes_cbc_256");

        if (!EVP_EncryptFinal_ex(&ctx, &(*out)[updated], &finalized))
          throw runtime_error("EVP_EncryptFinal_ex() failed in aes_cbc_256");
      } else {
        if (EVP_DecryptInit_ex(&ctx, cipher, NULL, key->get_key()->get(), key->get_iv()->get()) == 0)
          throw runtime_error("EVP_DecryptInit_ex() failed in aes_cbc_256");

        if (!EVP_DecryptUpdate(&ctx, &(*out)[0], &updated, in, size))
          throw runtime_error("EVP_DecryptUpdate() failed in aes_cbc_256");

        if (!EVP_DecryptFinal_ex(&ctx, &(*out)[updated], &finalized))
          throw runtime_error("EVP_DecryptFinal_ex() failed in aes_cbc_256");
      }

      out->resize(updated + finalized);

    } catch (...) {
      EVP_CIPHER_CTX_cleanup(&ctx);
      throw;
    }

    EVP_CIPHER_CTX_cleanup(&ctx);
  #endif
}
