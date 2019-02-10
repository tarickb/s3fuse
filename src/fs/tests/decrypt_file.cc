#include <inttypes.h>
#include <stdio.h>
#include <string.h>
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
  try {
    if (argc != 3) {
      fprintf(stderr, "usage: %s <input-prefix> <out-file>\n", argv[0]);
      return 1;
    }

    const std::string in_prefix = argv[1];

    FILE *f_out = fopen(argv[2], "w");
    FILE *f_in = fopen((in_prefix + ".s3_out").c_str(), "r");
    FILE *f_meta = fopen((in_prefix + ".s3_meta").c_str(), "r");
    if (!f_in || !f_out || !f_meta) {
      fprintf(stderr, "failed to open input/output file(s)\n");
      return 1;
    }

    auto file_key = s3::crypto::SymmetricKey::Empty();
    auto meta_key = s3::crypto::SymmetricKey::Empty();
    auto v_key = s3::crypto::Buffer::Empty();
    std::string root_hash, meta;
    size_t file_size = 0;
    std::string ref_meta;

    while (true) {
      char buf[1024];
      if (!fgets(buf, 1024, f_meta)) break;
      buf[strlen(buf) - 1] = '\0';  // strip trailing newline
      std::string line = buf;

      size_t pos = line.find(": ");
      if (pos == std::string::npos) {
        fprintf(stderr, "malformed input: %s\n", line.c_str());
        return 1;
      }

      std::string prefix = line.substr(0, pos);
      line = line.substr(pos + 2);

      if (prefix == "v_key") {
        v_key = s3::crypto::Buffer::FromHexString(line);
      } else if (prefix == "iv") {
        meta_key = s3::crypto::SymmetricKey::Create(
            v_key, s3::crypto::Buffer::FromHexString(line));
      } else if (prefix == "size") {
        file_size = std::stoull(line);
      } else if (prefix == "meta") {
        ref_meta = line;
      } else if (prefix == "meta_enc") {
        std::string meta =
            s3::crypto::Cipher::DecryptAsString<s3::crypto::AesCbc256WithPkcs,
                                                s3::crypto::Hex>(meta_key,
                                                                 line);
        if (meta != ref_meta) {
          fprintf(stderr, "meta mismatch\n");
          return 1;
        }

        pos = meta.find('#');
        if (pos == std::string::npos) {
          fprintf(stderr, "malformed meta: %s\n", meta.c_str());
          return 1;
        }

        file_key = s3::crypto::SymmetricKey::FromString(meta.substr(0, pos));
        root_hash = meta.substr(pos + 1);
      } else if (prefix == "f_key" || prefix == "root_hash" ||
                 prefix == "meta") {
        // ignore
      } else {
        fprintf(stderr, "unknown prefix: %s\n", prefix.c_str());
        return 1;
      }
    }

    s3::crypto::HashList<s3::crypto::Sha256> hashes(file_size);

    size_t offset = 0;
    while (true) {
      uint8_t buf_in[HASH_BLOCK_SIZE], buf_out[HASH_BLOCK_SIZE];
      size_t read_count = fread(buf_in, 1, HASH_BLOCK_SIZE, f_in);

      s3::crypto::AesCtr256::Decrypt(file_key, buf_in, read_count, buf_out);
      if (fwrite(buf_out, read_count, 1, f_out) != 1) {
        fprintf(stderr, "failed to write to output file\n");
        return 1;
      }

      hashes.ComputeHash(offset, buf_out, read_count);
      offset += read_count;
      if (read_count < HASH_BLOCK_SIZE) break;
    }

    if (root_hash != hashes.GetRootHash<s3::crypto::Hex>()) {
      fprintf(stderr, "hash mismatch\n");
      return 1;
    }

    fprintf(stderr, "done\n");
    return 0;

  } catch (const std::exception &e) {
    fprintf(stderr, "caught exception: %s\n", e.what());
  }

  return 1;
}
