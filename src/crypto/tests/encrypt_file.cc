#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdio.h>
#include <sys/stat.h>

#include <iostream>
#include <string>

#include "crypto/aes_cbc_256.h"
#include "crypto/aes_ctr_256.h"
#include "crypto/base64.h"
#include "crypto/cipher.h"
#include "crypto/hash_list.h"
#include "crypto/sha256.h"
#include "crypto/symmetric_key.h"

using std::cerr;
using std::endl;
using std::string;

using s3::crypto::aes_cbc_256;
using s3::crypto::aes_ctr_256;
using s3::crypto::base64;
using s3::crypto::buffer;
using s3::crypto::cipher;
using s3::crypto::hash_list;
using s3::crypto::hex;
using s3::crypto::sha256;
using s3::crypto::symmetric_key;

const size_t HASH_BLOCK_SIZE = 32 * 1024;

int main(int argc, char **argv)
{
  string out_prefix;
  symmetric_key::ptr file_key, meta_key;
  FILE *f_in, *f_out, *f_meta;
  buffer::ptr v_key;
  hash_list<sha256>::ptr hashes;
  struct stat s;
  aes_ctr_256::ptr ctr_enc;
  size_t part_num = 0;
  string root_hash, meta;

  if (argc != 4) {
    cerr << "usage: " << argv[0] << " <v-key> <in-file> <output-prefix>" << endl;
    return 1;
  }

  v_key = buffer::from_string(argv[1]);
  out_prefix = argv[3];

  f_in = fopen(argv[2], "r");
  f_out = fopen((out_prefix + ".s3_out").c_str(), "w");
  f_meta = fopen((out_prefix + ".s3_meta").c_str(), "w");

  if (!f_in || !f_out || !f_meta) {
    cerr << "failed to open input/output file(s)" << endl;
    return 1;
  }

  file_key = symmetric_key::generate<aes_ctr_256>();
  meta_key = symmetric_key::generate<aes_cbc_256>(v_key);

  fprintf(f_meta, "v_key: %s\n", v_key->to_string().c_str());
  fprintf(f_meta, "iv: %s\n", meta_key->get_iv()->to_string().c_str());
  fprintf(f_meta, "f_key: %s\n", file_key->to_string().c_str());

  fstat(fileno(f_in), &s);
  hashes.reset(new hash_list<sha256>((s.st_size + HASH_BLOCK_SIZE - 1) / HASH_BLOCK_SIZE));

  fprintf(f_meta, "size: %" PRId64 "\n", s.st_size);

  ctr_enc = aes_ctr_256::create(file_key);

  while (true) {
    uint8_t buf_in[HASH_BLOCK_SIZE], buf_out[HASH_BLOCK_SIZE];
    size_t read_count = 0;

    read_count = fread(buf_in, 1, HASH_BLOCK_SIZE, f_in);

    ctr_enc->encrypt(buf_in, read_count, buf_out);

    if (fwrite(buf_out, read_count, 1, f_out) != 1) {
      cerr << "failed to write to output file" << endl;
      return 1;
    }

    hashes->set_hash_of_part(part_num++, buf_in, read_count);

    if (read_count < HASH_BLOCK_SIZE)
      break;
  }

  root_hash = hashes->get_root_hash<hex>();
  meta = file_key->to_string() + "#" + root_hash;

  fprintf(f_meta, "root_hash: %s\n", root_hash.c_str());
  fprintf(f_meta, "meta: %s\n", meta.c_str());
  fprintf(f_meta, "meta_enc: %s\n", cipher::encrypt<aes_cbc_256, hex>(meta_key, meta).c_str());

  return 0;
}
