#include <gtest/gtest.h>

#include "crypto/aes_cbc_256.h"
#include "crypto/aes_ctr_256.h"
#include "crypto/symmetric_key.h"

using std::runtime_error;
using std::string;

using s3::crypto::aes_cbc_256;
using s3::crypto::aes_ctr_256;
using s3::crypto::symmetric_key;

namespace
{
  const int TEST_SIZES[] = { 1, 2, 4, 8, 16, 32, 64 };
  const int TEST_COUNT = sizeof(TEST_SIZES) / sizeof(TEST_SIZES[0]);

  void test_string(const string &str, size_t key_len, size_t iv_len)
  {
    size_t colon = str.find(':');

    ASSERT_FALSE(str.empty());
    ASSERT_NE(colon, string::npos);
    ASSERT_LT(colon, str.size() - 1);

    // x2 for hex encoding
    EXPECT_EQ(key_len * 2 + 1 + iv_len * 2, str.size());
    EXPECT_EQ(key_len * 2, colon);
    EXPECT_EQ(iv_len * 2, str.size() - colon - 1);
  }

  template <class cipher>
  void test_default()
  {
    test_string(
      symmetric_key::generate<cipher>()->to_string(),
      cipher::DEFAULT_KEY_LEN,
      cipher::IV_LEN);
  }

  template <class cipher>
  void test_varying(int key_len)
  {
    test_string(
      symmetric_key::generate<cipher>(key_len)->to_string(),
      key_len,
      cipher::IV_LEN);
  }
}

TEST(symmetric_key, aes_cbc_256_no_zero)
{
  EXPECT_THROW(symmetric_key::generate<aes_cbc_256>(0), runtime_error);
}

TEST(symmetric_key, aes_ctr_256_no_zero)
{
  EXPECT_THROW(symmetric_key::generate<aes_ctr_256>(0), runtime_error);
}

TEST(symmetric_key, invalid_string)
{
  EXPECT_THROW(symmetric_key::from_string(""), runtime_error);
}

TEST(symmetric_key, aes_cbc_256_from_string)
{
  symmetric_key::ptr sk = symmetric_key::generate<aes_cbc_256>();
  string str = sk->to_string();

  EXPECT_EQ(str, symmetric_key::from_string(str)->to_string());
}

TEST(symmetric_key, aes_ctr_256_from_string)
{
  symmetric_key::ptr sk = symmetric_key::generate<aes_ctr_256>();
  string str = sk->to_string();

  EXPECT_EQ(str, symmetric_key::from_string(str)->to_string());
}

TEST(symmetric_key, aes_cbc_256_default)
{
  test_default<aes_cbc_256>();
}

TEST(symmetric_key, aes_ctr_256_default)
{
  test_default<aes_ctr_256>();
}

TEST(symmetric_key, aes_cbc_256_varying)
{
  for (int i = 0; i < TEST_COUNT; i++)
    test_varying<aes_cbc_256>(TEST_SIZES[i]);
}

TEST(symmetric_key, aes_ctr_256_varying)
{
  for (int i = 0; i < TEST_COUNT; i++)
    test_varying<aes_ctr_256>(TEST_SIZES[i]);
}
