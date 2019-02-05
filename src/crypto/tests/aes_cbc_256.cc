#include <gtest/gtest.h>

#include "crypto/aes_cbc_256.h"
#include "crypto/cipher.h"
#include "crypto/hex.h"
#include "crypto/symmetric_key.h"

namespace s3 {
namespace crypto {
namespace tests {

TEST(aes_cbc_256, invalid_iv_len) {
  symmetric_key::ptr sk = symmetric_key::from_string("aabbccddeeff:aa");

  // parens required because of template-args-in-macros weirdness
  EXPECT_THROW((cipher::encrypt<aes_cbc_256_with_pkcs, hex>(sk, "foo")),
               std::runtime_error);
}

TEST(aes_cbc_256, decrypt_on_invalid_size) {
  symmetric_key::ptr sk = symmetric_key::generate<aes_cbc_256>();

  EXPECT_THROW((cipher::decrypt<aes_cbc_256_with_pkcs, hex>(sk, "aa")),
               std::runtime_error);
}

TEST(aes_cbc_256, decode_non_null_terminated_string) {
  uint8_t in[] = {0xaa, 0xbb, 0xcc}; // no terminating null
  std::vector<uint8_t> out;

  symmetric_key::ptr sk = symmetric_key::generate<aes_cbc_256>();

  cipher::encrypt<aes_cbc_256_with_pkcs>(sk, in, sizeof(in), &out);

  EXPECT_THROW(cipher::decrypt<aes_cbc_256_with_pkcs>(sk, &out[0], out.size()),
               std::runtime_error);
}

} // namespace tests
} // namespace crypto
} // namespace s3
