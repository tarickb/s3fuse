#include <gtest/gtest.h>

#include "crypto/base64.h"
#include "crypto/encoder.h"
#include "crypto/hex.h"
#include "crypto/hex_with_quotes.h"

using std::string;
using std::vector;

using namespace s3::crypto;

namespace
{
  const char *KAT_INPUT[] = { "", "hello world!", "11" /* shouldn't pad */, "1234" /* should pad */ };
  const char *KAT_OUT_HEX[] = { "00", "68656c6c6f20776f726c642100", "313100", "3132333400" };
  const char *KAT_OUT_HEX_QUOTE[] = { "\"00\"", "\"68656c6c6f20776f726c642100\"", "\"313100\"", "\"3132333400\"" };
  const char *KAT_OUT_B64[] = { "AA==", "aGVsbG8gd29ybGQhAA==", "MTEA", "MTIzNAA=" };

  const size_t KAT_COUNT = sizeof(KAT_INPUT) / sizeof(KAT_INPUT[0]);

  const int TEST_SIZES[] = { 1, 2, 3, 4, 5, 1023, 2048, 12345, 1048575, 1048576, 9999999 };
  const int TEST_COUNT = sizeof(TEST_SIZES) / sizeof(TEST_SIZES[0]);

  template <class encoding>
  void encode_kat(const char **kat)
  {
    for (size_t i = 0; i < KAT_COUNT; i++)
      EXPECT_EQ(string(kat[i]), encoder::encode<encoding>(KAT_INPUT[i]));
  }

  template <class encoding>
  void decode_kat(const char **kat)
  {
    for (size_t i = 0; i < KAT_COUNT; i++) {
      vector<uint8_t> dec;

      encoder::decode<encoding>(kat[i], &dec);
      EXPECT_STREQ(KAT_INPUT[i], reinterpret_cast<const char *>(&dec[0]));
    }
  }

  template <class encoding>
  void run_random()
  {
    for (int test = 0; test < TEST_COUNT; test++) {
      int test_size = TEST_SIZES[test];
      vector<uint8_t> in(test_size), out;
      string enc;
      bool diff = false;

      srand(time(NULL));

      for (int i = 0; i < test_size; i++)
        in[i] = rand() % UINT8_MAX;

      enc = encoder::encode<encoding>(in);
      encoder::decode<encoding>(enc, &out);

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
}

TEST(hex, encode_known_answers)
{
  encode_kat<hex>(KAT_OUT_HEX);
}

TEST(hex, decode_known_answers)
{
  decode_kat<hex>(KAT_OUT_HEX);
}

TEST(hex, random)
{
  run_random<hex>();
}

TEST(hex_with_quotes, encode_known_answers)
{
  encode_kat<hex_with_quotes>(KAT_OUT_HEX_QUOTE);
}

TEST(hex_with_quotes, decode_known_answers)
{
  decode_kat<hex_with_quotes>(KAT_OUT_HEX_QUOTE);
}

TEST(hex_with_quotes, random)
{
  run_random<hex_with_quotes>();
}

TEST(base64, encode_known_answers)
{
  encode_kat<base64>(KAT_OUT_B64);
}

TEST(base64, decode_known_answers)
{
  decode_kat<base64>(KAT_OUT_B64);
}

TEST(base64, random)
{
  run_random<base64>();
}
