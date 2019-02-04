#include <gtest/gtest.h>

#include "crypto/aes_ctr_256.h"
#include "crypto/symmetric_key.h"

namespace s3 { namespace crypto {
  namespace tests {

TEST(aes_ctr_256, invalid_iv_len)
{
  uint8_t in[aes_ctr_256::BLOCK_LEN];
  uint8_t out[aes_ctr_256::BLOCK_LEN];

  symmetric_key::ptr sk = symmetric_key::from_string("aabbccddeeff:aa");

  EXPECT_THROW(aes_ctr_256::encrypt(sk, in, aes_ctr_256::BLOCK_LEN, out), std::runtime_error);
}

TEST(aes_ctr_256, bad_byte_offset)
{
  uint8_t in[aes_ctr_256::BLOCK_LEN];
  uint8_t out[aes_ctr_256::BLOCK_LEN];

  symmetric_key::ptr sk = symmetric_key::generate<aes_ctr_256>();

  EXPECT_THROW(aes_ctr_256::encrypt_with_byte_offset(sk, 1, in, aes_ctr_256::BLOCK_LEN, out), std::runtime_error);
}

} } }
