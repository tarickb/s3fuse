#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdio.h>
#include <sys/stat.h>

#include <iostream>
#include <string>

#include "crypto/aes_cbc_256.h"
#include "crypto/aes_ctr_256.h"
#include "crypto/cipher.h"
#include "crypto/hash_list.h"
#include "crypto/sha256.h"
#include "crypto/symmetric_key.h"

const size_t HASH_BLOCK_SIZE =
    s3::crypto::hash_list<s3::crypto::sha256>::CHUNK_SIZE;

int main(int argc, char **argv) {
  std::string out_prefix;
  s3::crypto::symmetric_key::ptr file_key, meta_key;
  FILE *f_in, *f_out, *f_meta;
  s3::crypto::buffer::ptr v_key;
  s3::crypto::hash_list<s3::crypto::sha256>::ptr hashes;
  struct stat s;
  size_t offset = 0;
  std::string root_hash, meta;

  if (argc != 4) {
    std::cerr << "usage: " << argv[0] << " <v-key> <in-file> <output-prefix>"
              << std::endl;
    return 1;
  }

  v_key = s3::crypto::buffer::from_string(argv[1]);
  out_prefix = argv[3];

  f_in = fopen(argv[2], "r");
  f_out = fopen((out_prefix + ".s3_out").c_str(), "w");
  f_meta = fopen((out_prefix + ".s3_meta").c_str(), "w");

  if (!f_in || !f_out || !f_meta) {
    std::cerr << "failed to open input/output file(s)" << std::endl;
    return 1;
  }

  file_key = s3::crypto::symmetric_key::generate<s3::crypto::aes_ctr_256>();
  meta_key =
      s3::crypto::symmetric_key::generate<s3::crypto::aes_cbc_256_with_pkcs>(
          v_key);

  fprintf(f_meta, "v_key: %s\n", v_key->to_string().c_str());
  fprintf(f_meta, "iv: %s\n", meta_key->get_iv()->to_string().c_str());
  fprintf(f_meta, "f_key: %s\n", file_key->to_string().c_str());

  fstat(fileno(f_in), &s);
  hashes.reset(new s3::crypto::hash_list<s3::crypto::sha256>(s.st_size));

  fprintf(f_meta, "size: %" PRId64 "\n", s.st_size);

  while (true) {
    uint8_t buf_in[HASH_BLOCK_SIZE], buf_out[HASH_BLOCK_SIZE];
    size_t read_count = 0;

    read_count = fread(buf_in, 1, HASH_BLOCK_SIZE, f_in);

    s3::crypto::aes_ctr_256::encrypt(file_key, buf_in, read_count, buf_out);

    if (fwrite(buf_out, read_count, 1, f_out) != 1) {
      std::cerr << "failed to write to output file" << std::endl;
      return 1;
    }

    hashes->compute_hash(offset, buf_in, read_count);
    offset += read_count;

    if (read_count < HASH_BLOCK_SIZE)
      break;
  }

  root_hash = hashes->get_root_hash<s3::crypto::hex>();
  meta = file_key->to_string() + "#" + root_hash;

  fprintf(f_meta, "root_hash: %s\n", root_hash.c_str());
  fprintf(f_meta, "meta: %s\n", meta.c_str());
  fprintf(f_meta, "meta_enc: %s\n",
          s3::crypto::cipher::encrypt<s3::crypto::aes_cbc_256_with_pkcs,
                                      s3::crypto::hex>(meta_key, meta)
              .c_str());

  return 0;
}
