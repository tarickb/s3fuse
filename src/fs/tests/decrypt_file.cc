#include <inttypes.h>
#include <stdio.h>
#include <string.h>
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
  try {
    std::string in_prefix;
    s3::crypto::symmetric_key::ptr file_key, meta_key;
    FILE *f_in, *f_out, *f_meta;
    s3::crypto::buffer::ptr v_key;
    s3::crypto::hash_list<s3::crypto::sha256>::ptr hashes;
    size_t offset = 0;
    std::string root_hash, meta;
    size_t file_size = 0;
    std::string ref_meta;

    if (argc != 3) {
      std::cerr << "usage: " << argv[0] << " <input-prefix> <out-file>"
                << std::endl;
      return 1;
    }

    in_prefix = argv[1];

    f_out = fopen(argv[2], "w");
    f_in = fopen((in_prefix + ".s3_out").c_str(), "r");
    f_meta = fopen((in_prefix + ".s3_meta").c_str(), "r");

    if (!f_in || !f_out || !f_meta) {
      std::cerr << "failed to open input/output file(s)" << std::endl;
      return 1;
    }

    while (true) {
      char buf[1024];
      std::string line, prefix;
      size_t pos;

      if (!fgets(buf, 1024, f_meta))
        break;

      buf[strlen(buf) - 1] = '\0'; // strip trailing newline

      line = buf;
      pos = line.find(": ");

      if (pos == std::string::npos) {
        std::cerr << "malformed input: " << line << std::endl;
        return 1;
      }

      prefix = line.substr(0, pos);
      line = line.substr(pos + 2);

      if (prefix == "v_key") {
        v_key = s3::crypto::buffer::from_string(line);
      } else if (prefix == "iv") {
        meta_key = s3::crypto::symmetric_key::create(
            v_key, s3::crypto::buffer::from_string(line));
      } else if (prefix == "size") {
        file_size = std::stoull(line);
      } else if (prefix == "meta") {
        ref_meta = line;
      } else if (prefix == "meta_enc") {
        std::string meta =
            s3::crypto::cipher::decrypt<s3::crypto::aes_cbc_256_with_pkcs,
                                        s3::crypto::hex>(meta_key, line);

        if (meta != ref_meta) {
          std::cerr << "meta mismatch" << std::endl;
          return 1;
        }

        pos = meta.find('#');

        if (pos == std::string::npos) {
          std::cerr << "malformed meta: " << meta << std::endl;
          return 1;
        }

        file_key = s3::crypto::symmetric_key::from_string(meta.substr(0, pos));
        root_hash = meta.substr(pos + 1);
      } else if (prefix == "f_key" || prefix == "root_hash" ||
                 prefix == "meta") {
        // ignore
      } else {
        std::cerr << "unknown prefix: " << prefix << std::endl;
        return 1;
      }
    }

    hashes.reset(new s3::crypto::hash_list<s3::crypto::sha256>(file_size));

    while (true) {
      uint8_t buf_in[HASH_BLOCK_SIZE], buf_out[HASH_BLOCK_SIZE];
      size_t read_count = 0;

      read_count = fread(buf_in, 1, HASH_BLOCK_SIZE, f_in);

      s3::crypto::aes_ctr_256::decrypt(file_key, buf_in, read_count, buf_out);

      if (fwrite(buf_out, read_count, 1, f_out) != 1) {
        std::cerr << "failed to write to output file" << std::endl;
        return 1;
      }

      hashes->compute_hash(offset, buf_out, read_count);
      offset += read_count;

      if (read_count < HASH_BLOCK_SIZE)
        break;
    }

    if (root_hash != hashes->get_root_hash<s3::crypto::hex>()) {
      std::cerr << "hash mismatch" << std::endl;
      return 1;
    }

    std::cout << "done" << std::endl;

    return 0;

  } catch (const std::exception &e) {
    std::cerr << "caught exception: " << e.what() << std::endl;
  }

  return 1;
}
