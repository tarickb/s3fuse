#include <gtest/gtest.h>

#include "crypto/aes_cbc_256.h"
#include "crypto/aes_ctr_256.h"
#include "crypto/symmetric_key.h"

namespace s3 {
namespace crypto {
namespace tests {

namespace {
constexpr int TEST_SIZES[] = {1, 2, 4, 8, 16, 32, 64};
constexpr int TEST_COUNT = sizeof(TEST_SIZES) / sizeof(TEST_SIZES[0]);

void TestString(const std::string &str, size_t key_len, size_t iv_len) {
  size_t colon = str.find(':');

  ASSERT_FALSE(str.empty());
  ASSERT_NE(colon, std::string::npos);
  ASSERT_LT(colon, str.size() - 1);

  // x2 for hex encoding
  EXPECT_EQ(key_len * 2 + 1 + iv_len * 2, str.size());
  EXPECT_EQ(key_len * 2, colon);
  EXPECT_EQ(iv_len * 2, str.size() - colon - 1);
}

template <class Cipher>
void TestDefault() {
  TestString(SymmetricKey::Generate<Cipher>().ToString(),
             Cipher::DEFAULT_KEY_LEN, Cipher::IV_LEN);
}

template <class Cipher>
void TestVarying(int key_len) {
  TestString(SymmetricKey::Generate<Cipher>(key_len).ToString(), key_len,
             Cipher::IV_LEN);
}
}  // namespace

TEST(SymmetricKey, AesCbc256NoZero) {
  EXPECT_THROW(SymmetricKey::Generate<AesCbc256>(0), std::runtime_error);
}

TEST(SymmetricKey, AesCtr256NoZero) {
  EXPECT_THROW(SymmetricKey::Generate<AesCtr256>(0), std::runtime_error);
}

TEST(SymmetricKey, InvalidString) {
  EXPECT_THROW(SymmetricKey::FromString(""), std::runtime_error);
}

TEST(SymmetricKey, AesCbc256FromString) {
  auto sk = SymmetricKey::Generate<AesCbc256>();
  std::string str = sk.ToString();
  EXPECT_EQ(str, SymmetricKey::FromString(str).ToString());
}

TEST(SymmetricKey, AesCtr256FromString) {
  auto sk = SymmetricKey::Generate<AesCtr256>();
  std::string str = sk.ToString();
  EXPECT_EQ(str, SymmetricKey::FromString(str).ToString());
}

TEST(SymmetricKey, AesCbc256Default) { TestDefault<AesCbc256>(); }

TEST(SymmetricKey, AesCtr256Default) { TestDefault<AesCtr256>(); }

TEST(SymmetricKey, AesCbc256Varying) {
  for (int i = 0; i < TEST_COUNT; i++) TestVarying<AesCbc256>(TEST_SIZES[i]);
}

TEST(SymmetricKey, AesCtr256Varying) {
  for (int i = 0; i < TEST_COUNT; i++) TestVarying<AesCtr256>(TEST_SIZES[i]);
}

}  // namespace tests
}  // namespace crypto
}  // namespace s3
