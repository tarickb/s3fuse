#include <cstdint>
#include <gtest/gtest.h>

#include "crypto/aes_cbc_256.h"
#include "crypto/cipher.h"
#include "crypto/symmetric_key.h"
#include "crypto/tests/random.h"

namespace s3 {
namespace crypto {
namespace tests {

namespace {
constexpr int TEST_SIZES[] = {0,
                              1,
                              2,
                              3,
                              5,
                              123,
                              256,
                              1023,
                              1024,
                              2 * 1024,
                              64 * 1024 - 1,
                              1024 * 1024 - 1,
                              2 * 1024 * 1024};
constexpr int TEST_COUNT = sizeof(TEST_SIZES) / sizeof(TEST_SIZES[0]);
}  // namespace

TEST(AesCbc256, RandomData) {
  for (int test = 0; test < TEST_COUNT; test++) {
    std::vector<uint8_t> out_enc, out_dec;
    size_t size = TEST_SIZES[test];

    const auto sk = SymmetricKey::Generate<AesCbc256WithPkcs>();

    const auto in = Random::Read(size);
    ASSERT_EQ(size, in.size()) << "with size = " << size;

    out_enc = Cipher::Encrypt<AesCbc256WithPkcs>(sk, &in[0], in.size());
    ASSERT_GE(out_enc.size(), in.size()) << "with size = " << size;

    out_dec =
        Cipher::Decrypt<AesCbc256WithPkcs>(sk, &out_enc[0], out_enc.size());
    ASSERT_EQ(in.size(), out_dec.size()) << "with size = " << size;

    bool diff = false;
    for (size_t i = 0; i < in.size(); i++) {
      if (in[i] != out_dec[i]) {
        diff = true;
        break;
      }
    }
    ASSERT_FALSE(diff) << "with size = " << size;
  }
}

}  // namespace tests
}  // namespace crypto
}  // namespace s3
