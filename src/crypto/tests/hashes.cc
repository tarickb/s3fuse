#include <iostream>

#include "crypto/encoder.h"
#include "crypto/hash.h"
#include "crypto/hex.h"
#include "crypto/hex_with_quotes.h"
#include "crypto/md5.h"
#include "crypto/sha256.h"

using std::cout;
using std::endl;
using std::string;
using std::vector;

using namespace s3::crypto;

int main(int argc, char **argv)
{
  string in = "hello world!";

  cout << "starting string: " << in << endl << endl;

  cout << "md5: " << hash::compute<md5, hex>(in) << endl;
  cout << "sha256: " << hash::compute<sha256, hex>(in) << endl;

  return 0;
}
