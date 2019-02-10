#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdio.h>
#include <sys/stat.h>

#include <string>

#include "crypto/aes_cbc_256.h"
#include "crypto/aes_ctr_256.h"
#include "crypto/cipher.h"
#include "crypto/hash_list.h"
#include "crypto/sha256.h"
#include "crypto/symmetric_key.h"

const size_t HASH_BLOCK_SIZE =
    s3::crypto::HashList<s3::crypto::Sha256>::CHUNK_SIZE;

int main(int argc, char **argv) {
  if (argc != 4) {
    fprintf(stderr, "usage: %s <v-key> <in-file> <output-prefix>\n", argv[0]);
    return 1;
  }

  auto v_key = s3::crypto::Buffer::FromHexString(argv[1]);
  std::string out_prefix = argv[3];

  FILE *f_in = fopen(argv[2], "r");
  FILE *f_out = fopen((out_prefix + ".s3_out").c_str(), "w");
  FILE *f_meta = fopen((out_prefix + ".s3_meta").c_str(), "w");
  if (!f_in || !f_out || !f_meta) {
    fprintf(stderr, "failed to open input/output file(s)\n");
    return 1;
  }

  auto file_key = s3::crypto::SymmetricKey::Generate<s3::crypto::AesCtr256>();
  auto meta_key =
      s3::crypto::SymmetricKey::Generate<s3::crypto::AesCbc256WithPkcs>(v_key);

  fprintf(f_meta, "v_key: %s\n", v_key.ToHexString().c_str());
  fprintf(f_meta, "iv: %s\n", meta_key.iv().ToHexString().c_str());
  fprintf(f_meta, "f_key: %s\n", file_key.ToString().c_str());

  struct stat s;
  fstat(fileno(f_in), &s);
  s3::crypto::HashList<s3::crypto::Sha256> hashes(s.st_size);

  fprintf(f_meta, "size: %" PRId64 "\n", s.st_size);

  size_t offset = 0;
  while (true) {
    uint8_t buf_in[HASH_BLOCK_SIZE], buf_out[HASH_BLOCK_SIZE];
    size_t read_count = fread(buf_in, 1, HASH_BLOCK_SIZE, f_in);
    s3::crypto::AesCtr256::Encrypt(file_key, buf_in, read_count, buf_out);

    if (fwrite(buf_out, read_count, 1, f_out) != 1) {
      fprintf(stderr, "failed to write to output file\n");
      return 1;
    }

    hashes.ComputeHash(offset, buf_in, read_count);
    offset += read_count;

    if (read_count < HASH_BLOCK_SIZE) break;
  }

  std::string root_hash = hashes.GetRootHash<s3::crypto::Hex>();
  std::string meta = file_key.ToString() + "#" + root_hash;

  fprintf(f_meta, "root_hash: %s\n", root_hash.c_str());
  fprintf(f_meta, "meta: %s\n", meta.c_str());
  fprintf(f_meta, "meta_enc: %s\n",
          s3::crypto::Cipher::Encrypt<s3::crypto::AesCbc256WithPkcs,
                                      s3::crypto::Hex>(meta_key, meta)
              .c_str());

  return 0;
}
