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
struct KnownAnswer {
  const char *plain;
  const char *hex;
  const char *hex_quote;
  const char *base64;
};

constexpr KnownAnswer KAT_TESTS[] = {
    {"", "00", "\"00\"", "AA=="},
    {"hello world!", "68656c6c6f20776f726c642100",
     "\"68656c6c6f20776f726c642100\"", "aGVsbG8gd29ybGQhAA=="},
    {"11" /* shouldn't pad */, "313100", "\"313100\"", "MTEA"},
    {"1234" /* should pad */, "3132333400", "\"3132333400\"", "MTIzNAA="}};

constexpr int TEST_SIZES[] = {1,    2,     3,       4,       5,      1023,
                              2048, 12345, 1048575, 1048576, 9999999};

template <class Encoding>
void EncodeKat(const char *KnownAnswer::*output) {
  for (const auto &kat : KAT_TESTS)
    EXPECT_EQ(Encoder::Encode<Encoding>(kat.plain), kat.*output);
}

template <class Encoding>
void DecodeKat(const char *KnownAnswer::*input) {
  for (const auto &kat : KAT_TESTS) {
    const auto dec = Encoder::Decode<Encoding>(kat.*input);
    EXPECT_STREQ(kat.plain, reinterpret_cast<const char *>(&dec[0]));
  }
}

template <class Encoding>
void RunRandom() {
  for (const int test_size : TEST_SIZES) {
    std::vector<uint8_t> in(test_size), out;
    std::string enc;
    bool diff = false;
    auto seed = time(nullptr);

    for (int i = 0; i < test_size; i++)
      in[i] = rand_r(&seed) % std::numeric_limits<uint8_t>::max();

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

TEST(Hex, EncodeKnownAnswers) { EncodeKat<Hex>(&KnownAnswer::hex); }

TEST(Hex, DecodeKnownAnswers) { DecodeKat<Hex>(&KnownAnswer::hex); }

TEST(Hex, Random) { RunRandom<Hex>(); }

TEST(HexWithQuotes, EncodeKnownAnswers) {
  EncodeKat<HexWithQuotes>(&KnownAnswer::hex_quote);
}

TEST(HexWithQuotes, DecodeKnownAnswers) {
  DecodeKat<HexWithQuotes>(&KnownAnswer::hex_quote);
}

TEST(HexWithQuotes, Random) { RunRandom<HexWithQuotes>(); }

TEST(HexWithQuotes, DecodeWithNoQuotes) {
  EXPECT_THROW(Encoder::Decode<HexWithQuotes>("input has no quotes"),
               std::runtime_error);
}

TEST(Base64, EncodeKnownAnswers) { EncodeKat<Base64>(&KnownAnswer::base64); }

TEST(Base64, DecodeKnownAnswers) { DecodeKat<Base64>(&KnownAnswer::base64); }

TEST(Base64, Random) { RunRandom<Base64>(); }

}  // namespace tests
}  // namespace crypto
}  // namespace s3
