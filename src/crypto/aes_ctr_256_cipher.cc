#include <string.h>

#include "crypto/aes_ctr_256_cipher.h"
#include "crypto/cipher_state.h"

using std::runtime_error;

using s3::crypto::aes_ctr_256_cipher;
using s3::crypto::cipher_state;

namespace
{
  // TODO: find another way to do this?
  inline void le_to_be(const uint8_t *in, size_t size, uint8_t *out)
  {
    for (size_t i = 0; i < size; i++)
      out[size - i - 1] = in[i];
  }
}

aes_ctr_256_cipher::aes_ctr_256_cipher(const cipher_state::ptr &state, uint64_t starting_block)
{
  if (state->get_iv_len() != IV_LEN)
    throw runtime_error("iv length is not valid for this cipher");

  #ifdef __APPLE__
    CCCryptorStatus r;
    uint8_t iv[BLOCK_LEN];

    memcpy(iv, state->get_iv(), IV_LEN);

    le_to_be(reinterpret_cast<uint8_t *>(&starting_block), sizeof(starting_block), iv + IV_LEN);

    r = CCCryptorCreateWithMode(
      kCCEncrypt, // use for both encryption and decryption because ctr is symmetric
      kCCModeCTR,
      kCCAlgorithmAES128,
      ccNoPadding,
      iv,
      state->get_key(),
      state->get_key_len(),
      NULL,
      0,
      0,
      kCCModeOptionCTR_BE,
      &_cryptor);

    if (r != kCCSuccess)
      throw runtime_error("call to CCCryptorCreateWithMode() failed");
  #else
    memset(_ecount_buf, 0, sizeof(_ecount_buf));

    memcpy(_iv, state->get_iv(), IV_LEN);
    le_to_be(reinterpret_cast<uint8_t *>(&starting_block), sizeof(starting_block), _iv + IV_LEN);

    if (AES_set_encrypt_key(state->get_key(), state->get_key_len() * 8 /* in bits */, &_key) != 0)
      throw runtime_error("failed to set AES encryption key");
  #endif
}

aes_ctr_256_cipher::~aes_ctr_256_cipher()
{
  #ifdef __APPLE__
    CCCryptorRelease(_cryptor);
  #endif
}
