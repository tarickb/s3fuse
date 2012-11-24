#include <stdio.h>

#include <iostream>
#include <vector>

#include "crypto/aes_cbc_256.h"
#include "crypto/cipher.h"
#include "crypto/symmetric_key.h"

using std::cerr;
using std::cout;
using std::endl;
using std::vector;

using s3::crypto::aes_cbc_256;
using s3::crypto::cipher;
using s3::crypto::symmetric_key;

const size_t CHUNK = 1024;

int run_aes(bool encrypt, const symmetric_key::ptr &sk, const char *file_in, const char *file_out)
{
  FILE *f_in = NULL, *f_out = NULL;
  vector<uint8_t> in, out;
  size_t r = 0, offset = 0;

  f_in = fopen(file_in, "r");
  
  if (!f_in) {
    cerr << "failed to open [" << file_in << "] for input" << endl;
    return 1;
  }

  f_out = fopen(file_out, "w");

  if (!f_out) {
    cerr << "failed to open [" << file_out << "] for output" << endl;
    return 1;
  }

  while (true) {
    in.resize(offset + CHUNK);
    r = fread(&in[offset], 1, CHUNK, f_in);

    offset += r;
    in.resize(offset);

    if (r < CHUNK)
      break;
  }

  cout << "using key " << sk->to_string() << endl;
  cout << "read " << in.size() << " bytes" << endl;

  if (encrypt)
    cipher::encrypt<aes_cbc_256>(sk, &in[0], in.size(), &out);
  else
    cipher::decrypt<aes_cbc_256>(sk, &in[0], in.size(), &out);

  cout << (encrypt ? "encrypted" : "decrypted") << " " << out.size() << " bytes" << endl;

  r = fwrite(&out[0], 1, out.size(), f_out);

  if (r != out.size()) {
    cerr << "failed to write output" << endl;
    return 1;
  }

  return 0;
}

int main(int argc, char **argv)
{
  try {
    if (argc == 3)
      return run_aes(true, symmetric_key::generate<aes_cbc_256>(), argv[1], argv[2]);
    else if (argc == 4)
      return run_aes(false, symmetric_key::from_string(argv[1]), argv[2], argv[3]);

  } catch (const std::exception &e) {
    cout << "caught exception: " << e.what() << endl;
    return 1;
  }

  cout
    << "usage:" << endl
    << "  " << argv[0] << " <file-in> <file-out>        encrypt <file-in> and write to <file-out>" << endl
    << "  " << argv[0] << " <key> <file-in> <file-out>  decrypt <file-in> using <key> and write to <file-out>" << endl;

  return 1;
}
