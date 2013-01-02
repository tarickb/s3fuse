#include <gtest/gtest.h>

#include "crypto/aes_cbc_256.h"
#include "crypto/cipher.h"
#include "crypto/symmetric_key.h"
#include "crypto/tests/random.h"

using std::vector;

using s3::crypto::aes_cbc_256_with_pkcs;
using s3::crypto::cipher;
using s3::crypto::symmetric_key;
using s3::crypto::tests::random;

namespace
{
  const int TEST_SIZES[] = { 0, 1, 2, 3, 5, 123, 256, 1023, 1024, 2 * 1024, 64 * 1024 - 1, 1024 * 1024 - 1, 2 * 1024 * 1024 };
  const int TEST_COUNT = sizeof(TEST_SIZES) / sizeof(TEST_SIZES[0]);
}

TEST(aes_cbc_256, random_data)
{
  for (int test = 0; test < TEST_COUNT; test++) {
    symmetric_key::ptr sk;
    vector<uint8_t> in, out_enc, out_dec;
    bool diff = false;
    size_t size = TEST_SIZES[test];

    sk = symmetric_key::generate<aes_cbc_256_with_pkcs>();

    random::read(size, &in);

    ASSERT_EQ(size, in.size()) << "with size = " << size;

    cipher::encrypt<aes_cbc_256_with_pkcs>(sk, &in[0], in.size(), &out_enc);

    ASSERT_GE(out_enc.size(), in.size()) << "with size = " << size;

    cipher::decrypt<aes_cbc_256_with_pkcs>(sk, &out_enc[0], out_enc.size(), &out_dec);

    ASSERT_EQ(in.size(), out_dec.size()) << "with size = " << size;

    for (size_t i = 0; i < in.size(); i++) {
      if (in[i] != out_dec[i]) {
        diff = true;
        break;
      }
    }

    ASSERT_FALSE(diff) << "with size = " << size;
  }
}
