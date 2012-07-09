#define __STDC_LIMIT_MACROS
#include <stdint.h>

#include <stdlib.h>
#include <unistd.h>

#include <iostream>

#include "util.h"

using namespace std;
using namespace s3;

int main(int argc, char **argv)
{
  const int TEST_SIZES[] = { 1, 2, 3, 4, 5, 1023, 2048, 12345, 1048575, 1048576, 9999999 };

  srand(time(NULL));

  for (unsigned int t = 0; t < sizeof(TEST_SIZES) / sizeof(TEST_SIZES[0]); t++) {
    const int TEST_SIZE = TEST_SIZES[t];
    vector<uint8_t> in(TEST_SIZE), out;
    string enc;

    for (int i = 0; i < TEST_SIZE; i++)
      in[i] = rand() % UINT8_MAX;

    enc = util::base64_encode(&in[0], TEST_SIZE);
    util::base64_decode(enc, &out);

    cout << "test " << t << " (" << TEST_SIZE << " bytes, " << enc.size() << " b64 bytes): ";

    for (int i = 0; i < TEST_SIZE; i++) {
      if (in[i] != out[i]) {
        cout << "failed on index " << i << endl;
        return 1;
      }
    }

    cout << "passed" << endl;
  }

  return 0;
}
