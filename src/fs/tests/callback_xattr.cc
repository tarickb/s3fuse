#include <gtest/gtest.h>

#include "fs/callback_xattr.h"

namespace s3 {
namespace fs {
namespace tests {

TEST(CallbackXAttr, GetAndSet) {
  std::string val = "default";
  auto xattr = CallbackXAttr::Create("test_key",
                                     [&val](std::string *s) {
                                       *s = val;
                                       return 0;
                                     },
                                     [&val](std::string s) {
                                       val = s;
                                       return 0;
                                     },
                                     XAttr::XM_DEFAULT);
  ASSERT_TRUE(xattr);
  EXPECT_EQ(xattr->key(), "test_key");
  char buf[256];
  ASSERT_EQ(xattr->GetValue(buf, 256), val.size());
  EXPECT_EQ(std::string(buf), "default");
  std::string new_val = "new_val";
  ASSERT_EQ(xattr->SetValue(new_val.c_str(), new_val.size()), 0);
  EXPECT_EQ(val, new_val);
}

TEST(CallbackXAttr, Errors) {
  auto xattr = CallbackXAttr::Create(
      "test_key", [](std::string *s) { return -EIO; },
      [](std::string s) { return -EEXIST; }, XAttr::XM_DEFAULT);
  ASSERT_TRUE(xattr);
  EXPECT_EQ(xattr->key(), "test_key");
  EXPECT_EQ(xattr->GetValue(nullptr, 0), -EIO);
  EXPECT_EQ(xattr->SetValue(nullptr, 0), -EEXIST);
}

TEST(CallbackXAttr, GetReturnsLength) {
  std::string val = "xyz";
  auto xattr =
      CallbackXAttr::Create("test_key",
                            [val](std::string *s) {
                              *s = val;
                              return 0;
                            },
                            [](std::string s) { return 0; }, XAttr::XM_DEFAULT);
  ASSERT_TRUE(xattr);
  EXPECT_EQ(xattr->key(), "test_key");
  EXPECT_EQ(xattr->GetValue(nullptr, 0), val.size());
  char buf[2] = {0, 0};
  EXPECT_EQ(xattr->GetValue(buf, 2), -ERANGE);
  EXPECT_EQ(buf[0], val[0]);
  EXPECT_EQ(buf[1], val[1]);
}

TEST(CallbackXAttr, Serialization) {
  auto xattr =
      CallbackXAttr::Create("test_key",
                            [](std::string *s) {
                              *s = "abc";
                              return 0;
                            },
                            [](std::string s) { return 0; }, XAttr::XM_DEFAULT);
  ASSERT_TRUE(xattr);
  EXPECT_THROW(xattr->ToString(), std::runtime_error);
  std::string k, v;
  EXPECT_THROW(xattr->ToHeader(&k, &v), std::runtime_error);
}

}  // namespace tests
}  // namespace fs
}  // namespace s3
