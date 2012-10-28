#include <string.h>

#include "crypto/aes_ctr_256.h"
#include "crypto/symmetric_key.h"

using std::runtime_error;

using s3::crypto::aes_ctr_256;
using s3::crypto::symmetric_key;

namespace
{
  // TODO: find another way to do this?
  inline void le_to_be(const uint8_t *in, size_t size, uint8_t *out)
  {
    for (size_t i = 0; i < size; i++)
      out[size - i - 1] = in[i];
  }
}

aes_ctr_256::aes_ctr_256(const symmetric_key::ptr &key, uint64_t starting_block)
{
  if (key->get_iv()->size() != IV_LEN)
    throw runtime_error("iv length is not valid for aes_ctr_256");

  #ifdef __APPLE__
    CCCryptorStatus r;
    uint8_t iv[BLOCK_LEN];

    memcpy(iv, key->get_iv()->get(), IV_LEN);

    le_to_be(reinterpret_cast<uint8_t *>(&starting_block), sizeof(starting_block), iv + IV_LEN);

    r = CCCryptorCreateWithMode(
      kCCEncrypt, // use for both encryption and decryption because ctr is symmetric
      kCCModeCTR,
      kCCAlgorithmAES128,
      ccNoPadding,
      iv,
      key->get_key()->get(),
      key->get_key()->size(),
      NULL,
      0,
      0,
      kCCModeOptionCTR_BE,
      &_cryptor);

    if (r != kCCSuccess)
      throw runtime_error("call to CCCryptorCreateWithMode() failed in aes_ctr_256");
  #else
    memset(_ecount_buf, 0, sizeof(_ecount_buf));

    memcpy(_iv, key->get_iv()->get(), IV_LEN);
    le_to_be(reinterpret_cast<uint8_t *>(&starting_block), sizeof(starting_block), _iv + IV_LEN);

    if (AES_set_encrypt_key(key->get_key()->get(), key->get_key()->size() * 8 /* in bits */, &_key) != 0)
      throw runtime_error("failed to set encryption key for aes_ctr_256");
  #endif
}

aes_ctr_256::~aes_ctr_256()
{
  #ifdef __APPLE__
    CCCryptorRelease(_cryptor);
  #endif
}
