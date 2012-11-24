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

int main(int argc, char **argv)
{
  string in = "hello world!";
  string enc;
  vector<uint8_t> dec;

  cout << "starting string: " << in << endl << endl;

  enc = encoder::encode<hex>(in);
  cout << "hex: " << enc << endl;
  encoder::decode<hex>(enc, &dec);
  cout << "hex match: " << (in == reinterpret_cast<char *>(&dec[0]) ? "true" : "false") << endl << endl;

  enc = encoder::encode<hex_with_quotes>(in);
  cout << "hex_with_quotes: " << enc << endl;
  encoder::decode<hex_with_quotes>(enc, &dec);
  cout << "hex_with_quotes match: " << (in == reinterpret_cast<char *>(&dec[0]) ? "true" : "false") << endl << endl;

  enc = encoder::encode<base64>(in);
  cout << "base64: " << enc << endl;
  encoder::decode<base64>(enc, &dec);
  cout << "base64 match: " << (in == reinterpret_cast<char *>(&dec[0]) ? "true" : "false") << endl;

  return 0;
}
