#include <iostream>

#include "crypto/hash_list.h"
#include "crypto/hex.h"
#include "crypto/md5.h"
#include "crypto/sha256.h"

using std::cout;
using std::endl;
using std::string;
using std::vector;

using s3::crypto::hash_list;
using s3::crypto::hex;
using s3::crypto::md5;
using s3::crypto::sha256;

int main(int argc, char **argv)
{
  string inputs[] = { "input string 1", "input string 2" };

  hash_list<md5> md5_list(2);
  hash_list<sha256> sha_list(2);

  for (int i = 0; i < 2; i++) {
    const string &input = inputs[i];

    md5_list.set_hash_of_part(i, reinterpret_cast<const uint8_t *>(input.c_str()), input.size() + 1);
    sha_list.set_hash_of_part(i, reinterpret_cast<const uint8_t *>(input.c_str()), input.size() + 1);
  }

  cout << "md5 root: " << md5_list.get_root_hash<hex>() << endl;
  cout << "sha256 root: " << sha_list.get_root_hash<hex>() << endl;

  return 0;
}
