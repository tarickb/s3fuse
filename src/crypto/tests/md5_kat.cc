#include <cstdint>
#include <gtest/gtest.h>

#include "crypto/encoder.h"
#include "crypto/hash.h"
#include "crypto/hex.h"
#include "crypto/md5.h"

namespace s3 {
namespace crypto {
namespace tests {

namespace {
struct KnownAnswer {
  const char *message;
  const char *hash;
};

constexpr KnownAnswer TESTS[] = {
    // tests from http://www.nsrl.nist.gov/testdata/

    {"", "d41d8cd98f00b204e9800998ecf8427e"},

    {"616263", "900150983cd24fb0d6963f7d28e17f72"},

    {"68656c6c6f20776f726c6421", "fc3ff98e8c6a0d3087d515c0473f8677"},

    {"6162636462636465636465666465666765666768666768696768696a68696a6b696a6b6c6"
     "a6b6c6d6b6c6d6e6c6d6e6f6d6e6f706e6f7071",
     "8215ef0796a20bcaaae116d3876c664a"},
};
}  // namespace

TEST(Md5, KnownAnswers) {
  for (const auto &kat : TESTS) {
    std::vector<uint8_t> in;
    std::string hash;

    in = Encoder::Decode<Hex>(kat.message);
    ASSERT_EQ(strlen(kat.message) / 2, in.size())
        << "for kat = " << kat.message;

    hash = Hash::Compute<Md5, Hex>(in);
    EXPECT_EQ(std::string(kat.hash), hash) << "for kat = " << kat.message;
  }
}

}  // namespace tests
}  // namespace crypto
}  // namespace s3
