#include <gtest/gtest.h>

#include "crypto/aes_ctr_256.h"
#include "crypto/cipher.h"
#include "crypto/symmetric_key.h"
#include "crypto/tests/random.h"

using std::vector;

using s3::crypto::aes_ctr_256;
using s3::crypto::cipher;
using s3::crypto::symmetric_key;
using s3::crypto::tests::random;

namespace
{
  const int TEST_SIZES[] = { 0, 1, 2, 3, 5, 123, 256, 1023, 1024, 2 * 1024, 64 * 1024 - 1, 1024 * 1024 - 1, 2 * 1024 * 1024, 10 * 1024 * 1024 };
  const int TEST_COUNT = sizeof(TEST_SIZES) / sizeof(TEST_SIZES[0]);

  const size_t CHUNK_SIZE = 8 * 1024;
}

TEST(aes_ctr_256, random_data_sequential)
{
  for (int test = 0; test < TEST_COUNT; test++) {
    symmetric_key::ptr sk;
    vector<uint8_t> in, out_enc, out_dec;
    bool diff = false;
    size_t size = TEST_SIZES[test];

    sk = symmetric_key::generate<aes_ctr_256>();

    random::read(size, &in);

    ASSERT_EQ(size, in.size()) << "with size = " << size;

    out_enc.resize(size);
    out_dec.resize(size);

    for (size_t pos = 0; pos < size; pos += CHUNK_SIZE) {
      size_t remaining = size - pos;
      size_t this_chunk = (remaining > CHUNK_SIZE) ? CHUNK_SIZE : remaining;

      aes_ctr_256::encrypt(sk, &in[pos], this_chunk, &out_enc[pos]);
    }

    for (size_t pos = 0; pos < size; pos += CHUNK_SIZE) {
      size_t remaining = size - pos;
      size_t this_chunk = (remaining > CHUNK_SIZE) ? CHUNK_SIZE : remaining;

      aes_ctr_256::decrypt(sk, &out_enc[pos], this_chunk, &out_dec[pos]);
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
