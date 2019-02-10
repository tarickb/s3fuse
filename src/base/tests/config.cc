#include <gtest/gtest.h>
#include <fstream>
#include <stdexcept>

#include "base/config.h"

namespace s3 {
namespace base {
namespace tests {

TEST(Config, LoadFromInvalidFile) {
  EXPECT_THROW(Config::Init("/tmp/this shouldn't be a file"),
               std::runtime_error);
}

TEST(Config, LoadEmptyFile) {
  const char *TEMP_FILE = "/tmp/" PACKAGE_NAME ".test-empty";

  std::ofstream f(TEMP_FILE, std::ofstream::out | std::ofstream::trunc);
  EXPECT_THROW(Config::Init(TEMP_FILE), std::runtime_error);
  unlink(TEMP_FILE);
}

}  // namespace tests
}  // namespace base
}  // namespace s3
