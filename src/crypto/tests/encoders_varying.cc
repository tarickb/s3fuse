#define __STDC_LIMIT_MACROS
#include <stdint.h>

#include <stdlib.h>
#include <unistd.h>

#include <iostream>

#include "crypto/base64.h"
#include "crypto/encoder.h"
#include "crypto/hex.h"
#include "crypto/hex_with_quotes.h"

using std::cout;
using std::endl;
using std::string;
using std::vector;

using namespace s3::crypto;

template <class encoder_type>
void run_test(const char *encoder_name)
{
  const int TEST_SIZES[] = { 1, 2, 3, 4, 5, 1023, 2048, 12345, 1048575, 1048576, 9999999 };

  srand(time(NULL));

  for (unsigned int t = 0; t < sizeof(TEST_SIZES) / sizeof(TEST_SIZES[0]); t++) {
    const int TEST_SIZE = TEST_SIZES[t];
    vector<uint8_t> in(TEST_SIZE), out;
    string enc;

    for (int i = 0; i < TEST_SIZE; i++)
      in[i] = rand() % UINT8_MAX;

    enc = encoder::encode<encoder_type>(in);
    encoder::decode<encoder_type>(enc, &out);

    cout << encoder_name << ": test " << t << " (" << TEST_SIZE << " bytes, " << enc.size() << " encoded bytes): ";

    for (int i = 0; i < TEST_SIZE; i++) {
      if (in[i] != out[i]) {
        cout << encoder_name << ": failed on index " << i << endl;
        return;
      }
    }

    cout << encoder_name << ": passed" << endl;
  }
}

int main(int argc, char **argv)
{
  run_test<base64>("base64");
  run_test<hex>("hex");
  run_test<hex_with_quotes>("hex_with_quotes");

  return 0;
}
