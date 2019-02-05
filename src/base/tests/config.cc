#include <fstream>
#include <gtest/gtest.h>
#include <stdexcept>

#include "base/config.h"

namespace s3 {
namespace base {
namespace tests {

TEST(config, load_from_invalid_file) {
  EXPECT_THROW(config::init("/tmp/this shouldn't be a file"),
               std::runtime_error);
}

TEST(config, load_empty_file) {
  const char *TEMP_FILE = "/tmp/" PACKAGE_NAME ".test-empty";

  std::ofstream f(TEMP_FILE, std::ofstream::out | std::ofstream::trunc);

  EXPECT_THROW(config::init(TEMP_FILE), std::runtime_error);

  unlink(TEMP_FILE);
}

} // namespace tests
} // namespace base
} // namespace s3
