#include <gtest/gtest.h>

#include "crypto/aes_cbc_256.h"
#include "crypto/cipher.h"
#include "crypto/hex.h"
#include "crypto/symmetric_key.h"

using std::runtime_error;

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
