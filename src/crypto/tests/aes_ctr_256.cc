#include <gtest/gtest.h>

#include "crypto/aes_ctr_256.h"
#include "crypto/symmetric_key.h"

using std::runtime_error;

using s3::crypto::aes_ctr_256;
using s3::crypto::symmetric_key;

TEST(aes_ctr_256, invalid_iv_len)
{
  uint8_t in[aes_ctr_256::BLOCK_LEN];
  uint8_t out[aes_ctr_256::BLOCK_LEN];

  symmetric_key::ptr sk = symmetric_key::from_string("aabbccddeeff:aa");

  EXPECT_THROW(aes_ctr_256::encrypt(sk, in, aes_ctr_256::BLOCK_LEN, out), runtime_error);
}

TEST(aes_ctr_256, bad_byte_offset)
{
  uint8_t in[aes_ctr_256::BLOCK_LEN];
  uint8_t out[aes_ctr_256::BLOCK_LEN];

  symmetric_key::ptr sk = symmetric_key::generate<aes_ctr_256>();

  EXPECT_THROW(aes_ctr_256::encrypt_with_byte_offset(sk, 1, in, aes_ctr_256::BLOCK_LEN, out), runtime_error);
}
