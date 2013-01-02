#include <gtest/gtest.h>

#include "crypto/buffer.h"
#include "crypto/pbkdf2_sha1.h"

using std::string;

using s3::crypto::buffer;
using s3::crypto::pbkdf2_sha1;

namespace
{
  struct known_answer
  {
    const char *password;
    const char *salt;
    int rounds;
    int key_len;
    const char *output;
  };

  const known_answer TESTS[] = {
    // from http://tools.ietf.org/html/draft-josefsson-pbkdf2-test-vectors-06
    // (excluding the last test, since nulls in strings would complicate matters)

    { "password",
      "salt",
      1,
      20,
      "0c60c80f961f0e71f3a9b524af6012062fe037a6" },

    { "password",
      "salt",
      2,
      20,
      "ea6c014dc72d6f8ccd1ed92ace1d41f0d8de8957" },

    { "password",
      "salt",
      4096,
      20,
      "4b007901b765489abead49d926f721d065a429c1" },

    { "password",
      "salt",
      16777216,
      20,
      "eefe3d61cd4da4e4e9945b3d6ba2158c2634e984" },

    { "passwordPASSWORDpassword",
      "saltSALTsaltSALTsaltSALTsaltSALTsalt",
      4096,
      25,
      "3d2eec4fe41c849b80c8d83662c0e44a8b291a964cf2f07038" }
  };

  const int TEST_COUNT = sizeof(TESTS) / sizeof(TESTS[0]);
}

TEST(pbkdf2_sha1, known_answers)
{
  for (int test = 0; test < TEST_COUNT; test++) {
    const known_answer *kat = TESTS + test;
    string key_out, pretty_kat;
    buffer::ptr key;

    key = pbkdf2_sha1::derive(kat->password, kat->salt, kat->rounds, kat->key_len);
    key_out = key->to_string();

    EXPECT_EQ(string(kat->output), key_out) <<
      "password: " << kat->password << ", " <<
      "salt: " << kat->salt << ", " <<
      "rounds: " << kat->rounds << ", " <<
      "key len: " << kat->key_len;
  }
}
