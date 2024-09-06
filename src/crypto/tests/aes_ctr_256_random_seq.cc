#include <cstdint>
#include <gtest/gtest.h>

#include "crypto/aes_ctr_256.h"
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
                              2 * 1024 * 1024,
                              10 * 1024 * 1024};
constexpr int TEST_COUNT = sizeof(TEST_SIZES) / sizeof(TEST_SIZES[0]);

constexpr size_t CHUNK_SIZE = 8 * 1024;
}  // namespace

TEST(AesCtr256, RandomDataSequential) {
  for (int test = 0; test < TEST_COUNT; test++) {
    std::vector<uint8_t> in, out_enc, out_dec;
    bool diff = false;
    size_t size = TEST_SIZES[test];

    const auto sk = SymmetricKey::Generate<AesCtr256>();

    in = Random::Read(size);

    ASSERT_EQ(size, in.size()) << "with size = " << size;

    out_enc.resize(size);
    out_dec.resize(size);

    for (size_t pos = 0; pos < size; pos += CHUNK_SIZE) {
      size_t remaining = size - pos;
      size_t this_chunk = (remaining > CHUNK_SIZE) ? CHUNK_SIZE : remaining;

      AesCtr256::Encrypt(sk, &in[pos], this_chunk, &out_enc[pos]);
    }

    for (size_t pos = 0; pos < size; pos += CHUNK_SIZE) {
      size_t remaining = size - pos;
      size_t this_chunk = (remaining > CHUNK_SIZE) ? CHUNK_SIZE : remaining;

      AesCtr256::Decrypt(sk, &out_enc[pos], this_chunk, &out_dec[pos]);
    }

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
