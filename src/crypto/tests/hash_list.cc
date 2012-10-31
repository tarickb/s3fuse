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
  size_t buf_len = hash_list<sha256>::CHUNK_SIZE * 4 + 123;
  uint8_t *uninit_buffer = new uint8_t[buf_len];

  hash_list<md5> md5_list(buf_len);
  hash_list<sha256> sha_list(buf_len);

  for (int i = 0; i < 3; i++) {
    if (i == 0) {
      cout << "uninitialized buffer" << endl;
    } else if (i == 1) {
      cout << "modified at first byte" << endl;
      uninit_buffer[0] = 123;
    } else if (i == 2) {
      cout << "modified at last byte" << endl;
      uninit_buffer[buf_len - 1] = 123;
    }

    md5_list.compute_hash(0, uninit_buffer, buf_len);
    sha_list.compute_hash(0, uninit_buffer, buf_len);

    cout << "md5 root: " << md5_list.get_root_hash<hex>() << endl;
    cout << "sha256 root: " << sha_list.get_root_hash<hex>() << endl;
  }

  return 0;
}
