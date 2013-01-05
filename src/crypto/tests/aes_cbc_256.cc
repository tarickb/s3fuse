#include <gtest/gtest.h>

#include "crypto/aes_cbc_256.h"
#include "crypto/cipher.h"
#include "crypto/hex.h"
#include "crypto/symmetric_key.h"

using std::runtime_error;
using std::vector;

using s3::crypto::aes_cbc_256;
using s3::crypto::aes_cbc_256_with_pkcs;
using s3::crypto::cipher;
using s3::crypto::hex;
using s3::crypto::symmetric_key;

TEST(aes_cbc_256, invalid_iv_len)
{
  symmetric_key::ptr sk = symmetric_key::from_string("aabbccddeeff:aa");

  // parens required because of template-args-in-macros weirdness
  EXPECT_THROW((cipher::encrypt<aes_cbc_256_with_pkcs, hex>(sk, "foo")), runtime_error);
}

TEST(aes_cbc_256, decrypt_on_invalid_size)
{
  symmetric_key::ptr sk = symmetric_key::generate<aes_cbc_256>();

  EXPECT_THROW((cipher::decrypt<aes_cbc_256_with_pkcs, hex>(sk, "aa")), runtime_error);
}

TEST(aes_cbc_256, decode_non_null_terminated_string)
{
  uint8_t in[] = { 0xaa, 0xbb, 0xcc }; // no terminating null
  vector<uint8_t> out;

  symmetric_key::ptr sk = symmetric_key::generate<aes_cbc_256>();

  cipher::encrypt<aes_cbc_256_with_pkcs>(sk, in, sizeof(in), &out);

  EXPECT_THROW(cipher::decrypt<aes_cbc_256_with_pkcs>(sk, &out[0], out.size()), runtime_error);
}
