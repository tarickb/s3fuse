#include <gtest/gtest.h>

#include "crypto/aes_cbc_256.h"
#include "crypto/cipher.h"
#include "crypto/hex.h"
#include "crypto/symmetric_key.h"

namespace s3 {
namespace crypto {
namespace tests {

TEST(AesCbc256, InvalidIvLen) {
  const auto sk = SymmetricKey::FromString("aabbccddeeff:aa");

  // parens required because of template-args-in-macros weirdness
  EXPECT_THROW((Cipher::Encrypt<AesCbc256WithPkcs, Hex>(sk, "foo")),
               std::runtime_error);
}

TEST(AesCbc256, DecryptOnInvalidSize) {
  const auto sk = SymmetricKey::Generate<AesCbc256>();

  EXPECT_THROW((Cipher::DecryptAsString<AesCbc256WithPkcs, Hex>(sk, "aa")),
               std::runtime_error);
}

TEST(AesCbc256, DecodeNonNullTerminatedString) {
  uint8_t in[] = {0xaa, 0xbb, 0xcc};  // no terminating null

  const auto sk = SymmetricKey::Generate<AesCbc256>();

  auto out = Cipher::Encrypt<AesCbc256WithPkcs>(sk, in, sizeof(in));

  EXPECT_THROW(
      Cipher::DecryptAsString<AesCbc256WithPkcs>(sk, &out[0], out.size()),
      std::runtime_error);
}

}  // namespace tests
}  // namespace crypto
}  // namespace s3
