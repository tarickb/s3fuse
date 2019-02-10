#include <gtest/gtest.h>

#include "fs/static_xattr.h"

namespace s3 {
namespace fs {
namespace tests {

TEST(StaticXAttr, GetAndSet) {
  const std::string val = "value";
  auto xattr = StaticXAttr::FromString("test_key", val, XAttr::XM_VISIBLE);
  ASSERT_TRUE(xattr);
  EXPECT_EQ(xattr->key(), "test_key");
  char buf[256];
  ASSERT_EQ(xattr->GetValue(buf, 256), val.size());
  EXPECT_EQ(std::string(buf), val);
  std::string new_val = "new_val";
  ASSERT_EQ(xattr->SetValue(new_val.c_str(), new_val.size()), 0);
  ASSERT_EQ(xattr->GetValue(buf, 256), new_val.size());
  EXPECT_EQ(std::string(buf), new_val);
}

TEST(StaticXAttr, HideOnEmpty) {
  auto xattr = StaticXAttr::FromHeader("test_key", "", XAttr::XM_VISIBLE);
  ASSERT_TRUE(xattr);
  EXPECT_FALSE(xattr->is_visible());
}

TEST(StaticXAttr, InvalidKey) {
  const std::string key = "should_be_AN_INVALID_KEY";
  const std::string value = "abc";
  auto xattr = StaticXAttr::FromString(key, value, XAttr::XM_VISIBLE);
  ASSERT_TRUE(xattr);
  EXPECT_EQ(xattr->key(), key);
  std::string k, v;
  xattr->ToHeader(&k, &v);
  EXPECT_NE(key, k);
  EXPECT_NE(value, v);
}

TEST(StaticXAttr, GetReturnsLength) {
  std::string val = "abcdefghi";
  auto xattr = StaticXAttr::FromString("test_key", val, XAttr::XM_VISIBLE);
  ASSERT_TRUE(xattr);
  EXPECT_EQ(xattr->key(), "test_key");
  EXPECT_EQ(xattr->GetValue(nullptr, 0), val.size());
  char buf[2] = {0, 0};
  EXPECT_EQ(xattr->GetValue(buf, 2), -ERANGE);
  EXPECT_EQ(buf[0], val[0]);
  EXPECT_EQ(buf[1], val[1]);
}

}  // namespace tests
}  // namespace fs
}  // namespace s3
