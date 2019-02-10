#include <gtest/gtest.h>

#include "fs/mime_types.h"

namespace s3 {
namespace fs {
namespace tests {

TEST(MimeTypes, GetWithoutInit) {
  EXPECT_EQ(MimeTypes::GetTypeByExtension("txt"), "");
}

TEST(MimeTypes, CommonExtensions) {
  MimeTypes::Init();
  EXPECT_EQ(MimeTypes::GetTypeByExtension("txt"), "text/plain");
  EXPECT_EQ(MimeTypes::GetTypeByExtension("jpg"), "image/jpeg");
}

}  // namespace tests
}  // namespace fs
}  // namespace s3
