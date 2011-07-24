#include <stdio.h>
#include <openssl/rand.h>

#include <stdexcept>

#include "aes_256_cbc_cipher.h"
#include "util.h"

using namespace std;

using namespace s3;

namespace
{
  const int BUFFER_SIZE = 1024;
}

#define TO_STR_AGAIN(x) #x
#define TO_STR(x) TO_STR_AGAIN(x)

#define TEST_CRYPTO_SUCCESS(fn, ...) \
  do { \
    if (fn(__VA_ARGS__) != 1) \
      throw runtime_error("call to " #fn " at line " TO_STR(__LINE__) " failed."); \
  } while (0)

aes_256_cbc_cipher::aes_256_cbc_cipher(const uint8_t *key, const string &iv)
{
  EVP_CIPHER_CTX_init(&_enc);
  EVP_CIPHER_CTX_init(&_dec);

  try {
    if (iv.empty()) {
      _iv.resize(EVP_CIPHER_iv_length(EVP_aes_256_cbc()));
      TEST_CRYPTO_SUCCESS(RAND_bytes, &_iv[0], _iv.size());
      _iv_str = util::hex_encode(&_iv[0], _iv.size());

    } else {
      util::hex_decode(iv, &_iv);
      _iv_str = util::hex_encode(&_iv[0], _iv.size());

      if (_iv.size() < static_cast<size_t>(EVP_CIPHER_iv_length(EVP_aes_256_cbc())))
        throw runtime_error("IV not long enough.");
    }

    TEST_CRYPTO_SUCCESS(EVP_EncryptInit_ex, &_enc, EVP_aes_256_cbc(), NULL, key, &_iv[0]);
    TEST_CRYPTO_SUCCESS(EVP_DecryptInit_ex, &_dec, EVP_aes_256_cbc(), NULL, key, &_iv[0]);

  } catch (...) {
    EVP_CIPHER_CTX_cleanup(&_enc);
    EVP_CIPHER_CTX_cleanup(&_dec);

    throw;
  }
}

aes_256_cbc_cipher::~aes_256_cbc_cipher()
{
  EVP_CIPHER_CTX_cleanup(&_enc);
  EVP_CIPHER_CTX_cleanup(&_dec);
}

void aes_256_cbc_cipher::encrypt(int in_fd, int out_fd)
{
  uint8_t in_buf[BUFFER_SIZE], out_buf[BUFFER_SIZE + EVP_MAX_BLOCK_LENGTH];
  int r = 0, out_size = 0;

  TEST_CRYPTO_SUCCESS(EVP_EncryptInit_ex, &_enc, NULL, NULL, NULL, NULL);

  while ((r = read(in_fd, in_buf, BUFFER_SIZE)) > 0) {
    TEST_CRYPTO_SUCCESS(EVP_EncryptUpdate, &_enc, out_buf, &out_size, in_buf, r);

    if (write(out_fd, out_buf, out_size) != out_size)
      throw runtime_error("error while writing encrypted output.");
  }

  if (r != 0)
    throw runtime_error("error while reading plaintext input.");

  TEST_CRYPTO_SUCCESS(EVP_EncryptFinal_ex, &_enc, out_buf, &out_size);

  if (write(out_fd, out_buf, out_size) != out_size)
    throw runtime_error("error while finalizing encrypted output.");
}

void aes_256_cbc_cipher::decrypt(int in_fd, int out_fd)
{
  uint8_t in_buf[BUFFER_SIZE], out_buf[BUFFER_SIZE + EVP_MAX_BLOCK_LENGTH];
  int r = 0, out_size = 0;

  TEST_CRYPTO_SUCCESS(EVP_DecryptInit_ex, &_dec, NULL, NULL, NULL, NULL);

  while ((r = read(in_fd, in_buf, BUFFER_SIZE)) > 0) {
    TEST_CRYPTO_SUCCESS(EVP_DecryptUpdate, &_dec, out_buf, &out_size, in_buf, r);

    if (write(out_fd, out_buf, out_size) != out_size)
      throw runtime_error("error while writing plaintext output.");
  }

  if (r != 0)
    throw runtime_error("error while reading encrypted input.");

  TEST_CRYPTO_SUCCESS(EVP_DecryptFinal_ex, &_dec, out_buf, &out_size);

  if (write(out_fd, out_buf, out_size) != out_size)
    throw runtime_error("error while finalizing plaintext output.");
}

const string & aes_256_cbc_cipher::get_iv()
{
  return _iv_str;
}
