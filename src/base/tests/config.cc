#include <fstream>
#include <stdexcept>
#include <gtest/gtest.h>

#include "base/config.h"

using std::ofstream;
using std::runtime_error;

using s3::base::config;

TEST(config, load_from_invalid_file)
{
  EXPECT_THROW(config::init("/tmp/this shouldn't be a file"), runtime_error);
}

TEST(config, load_empty_file)
{
  const char *TEMP_FILE = "/tmp/s3fuse.test-empty";

  ofstream f(TEMP_FILE, ofstream::out | ofstream::trunc);

  EXPECT_THROW(config::init(TEMP_FILE), runtime_error);

  unlink(TEMP_FILE);
}
