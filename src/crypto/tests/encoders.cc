#include <gtest/gtest.h>
#include <limits>

#include "crypto/base64.h"
#include "crypto/encoder.h"
#include "crypto/hex.h"
#include "crypto/hex_with_quotes.h"

namespace s3 {
namespace crypto {
namespace tests {

namespace {
const char *KAT_INPUT[] = {"", "hello world!", "11" /* shouldn't pad */,
                           "1234" /* should pad */};
const char *KAT_OUT_HEX[] = {"00", "68656c6c6f20776f726c642100", "313100",
                             "3132333400"};
const char *KAT_OUT_HEX_QUOTE[] = {"\"00\"", "\"68656c6c6f20776f726c642100\"",
                                   "\"313100\"", "\"3132333400\""};
const char *KAT_OUT_B64[] = {"AA==", "aGVsbG8gd29ybGQhAA==", "MTEA",
                             "MTIzNAA="};

const size_t KAT_COUNT = sizeof(KAT_INPUT) / sizeof(KAT_INPUT[0]);

const int TEST_SIZES[] = {1,    2,     3,       4,       5,      1023,
                          2048, 12345, 1048575, 1048576, 9999999};
const int TEST_COUNT = sizeof(TEST_SIZES) / sizeof(TEST_SIZES[0]);

template <class Encoding>
void EncodeKat(const char **kat) {
  for (size_t i = 0; i < KAT_COUNT; i++)
    EXPECT_EQ(std::string(kat[i]), Encoder::Encode<Encoding>(KAT_INPUT[i]));
}

template <class Encoding>
void DecodeKat(const char **kat) {
  for (size_t i = 0; i < KAT_COUNT; i++) {
    const auto dec = Encoder::Decode<Encoding>(kat[i]);
    EXPECT_STREQ(KAT_INPUT[i], reinterpret_cast<const char *>(&dec[0]));
  }
}

template <class Encoding>
void RunRandom() {
  for (int test = 0; test < TEST_COUNT; test++) {
    int test_size = TEST_SIZES[test];
    std::vector<uint8_t> in(test_size), out;
    std::string enc;
    bool diff = false;

    srand(time(nullptr));

    for (int i = 0; i < test_size; i++)
      in[i] = rand() % std::numeric_limits<uint8_t>::max();

    enc = Encoder::Encode<Encoding>(in);
    out = Encoder::Decode<Encoding>(enc);
    ASSERT_EQ(in.size(), out.size()) << "with size = " << test_size;

    for (int i = 0; i < test_size; i++) {
      if (in[i] != out[i]) {
        diff = true;
        break;
      }
    }
    EXPECT_FALSE(diff) << "with size = " << test_size;
  }
}
}  // namespace

TEST(Hex, EncodeKnownAnswers) { EncodeKat<Hex>(KAT_OUT_HEX); }

TEST(Hex, DecodeKnownAnswers) { DecodeKat<Hex>(KAT_OUT_HEX); }

TEST(Hex, Random) { RunRandom<Hex>(); }

TEST(HexWithQuotes, EncodeKnownAnswers) {
  EncodeKat<HexWithQuotes>(KAT_OUT_HEX_QUOTE);
}

TEST(HexWithQuotes, DecodeKnownAnswers) {
  DecodeKat<HexWithQuotes>(KAT_OUT_HEX_QUOTE);
}

TEST(HexWithQuotes, Random) { RunRandom<HexWithQuotes>(); }

TEST(HexWithQuotes, DecodeWithNoQuotes) {
  EXPECT_THROW(Encoder::Decode<HexWithQuotes>("input has no quotes"),
               std::runtime_error);
}

TEST(Base64, EncodeKnownAnswers) { EncodeKat<Base64>(KAT_OUT_B64); }

TEST(Base64, DecodeKnownAnswers) { DecodeKat<Base64>(KAT_OUT_B64); }

TEST(Base64, Random) { RunRandom<Base64>(); }

}  // namespace tests
}  // namespace crypto
}  // namespace s3
