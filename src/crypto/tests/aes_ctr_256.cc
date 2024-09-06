#include <cstdint>
#include <gtest/gtest.h>

#include "crypto/aes_ctr_256.h"
#include "crypto/symmetric_key.h"

namespace s3 {
namespace crypto {
namespace tests {

TEST(AesCtr256, InvalidIvLen) {
  uint8_t in[AesCtr256::BLOCK_LEN];
  uint8_t out[AesCtr256::BLOCK_LEN];

  const auto sk = SymmetricKey::FromString("aabbccddeeff:aa");

  EXPECT_THROW(AesCtr256::Encrypt(sk, in, AesCtr256::BLOCK_LEN, out),
               std::runtime_error);
}

TEST(AesCtr256, BadByteOffset) {
  uint8_t in[AesCtr256::BLOCK_LEN];
  uint8_t out[AesCtr256::BLOCK_LEN];

  const auto sk = SymmetricKey::Generate<AesCtr256>();

  EXPECT_THROW(
      AesCtr256::EncryptWithByteOffset(sk, 1, in, AesCtr256::BLOCK_LEN, out),
      std::runtime_error);
}

}  // namespace tests
}  // namespace crypto
}  // namespace s3
