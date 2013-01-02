#include <gtest/gtest.h>

#include "crypto/encoder.h"
#include "crypto/hash.h"
#include "crypto/hex.h"
#include "crypto/md5.h"

using std::string;
using std::vector;

using s3::crypto::encoder;
using s3::crypto::hash;
using s3::crypto::hex;
using s3::crypto::md5;

namespace
{
  struct known_answer
  {
    const char *message;
    const char *hash;
  };

  const known_answer TESTS[] = {
    // tests from http://www.nsrl.nist.gov/testdata/

    { "",
      "d41d8cd98f00b204e9800998ecf8427e" },

    { "616263",
      "900150983cd24fb0d6963f7d28e17f72" },

    { "68656c6c6f20776f726c6421",
      "fc3ff98e8c6a0d3087d515c0473f8677" },

    { "6162636462636465636465666465666765666768666768696768696a68696a6b696a6b6c6a6b6c6d6b6c6d6e6c6d6e6f6d6e6f706e6f7071",
      "8215ef0796a20bcaaae116d3876c664a" },
  };

  const int TEST_COUNT = sizeof(TESTS) / sizeof(TESTS[0]);
}

TEST(md5, known_answers)
{
  for (int test = 0; test < TEST_COUNT; test++) {
    const known_answer *kat = TESTS + test;
    vector<uint8_t> in;
    string hash;

    encoder::decode<hex>(kat->message, &in);

    ASSERT_EQ(strlen(kat->message) / 2, in.size()) << "for kat = " << test;

    hash = hash::compute<md5, hex>(in);

    EXPECT_EQ(string(kat->hash), hash) << "for kat = " << test;
  }
}
