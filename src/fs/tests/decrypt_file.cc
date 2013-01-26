#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include <iostream>
#include <string>
#include <boost/lexical_cast.hpp>

#include "crypto/aes_cbc_256.h"
#include "crypto/aes_ctr_256.h"
#include "crypto/cipher.h"
#include "crypto/hash_list.h"
#include "crypto/sha256.h"
#include "crypto/symmetric_key.h"

using boost::lexical_cast;
using std::cerr;
using std::cout;
using std::endl;
using std::string;

using s3::crypto::aes_cbc_256_with_pkcs;
using s3::crypto::aes_ctr_256;
using s3::crypto::buffer;
using s3::crypto::cipher;
using s3::crypto::hash_list;
using s3::crypto::hex;
using s3::crypto::sha256;
using s3::crypto::symmetric_key;

const size_t HASH_BLOCK_SIZE = hash_list<sha256>::CHUNK_SIZE;

int main(int argc, char **argv)
{
  try {
    string in_prefix;
    symmetric_key::ptr file_key, meta_key;
    FILE *f_in, *f_out, *f_meta;
    buffer::ptr v_key;
    hash_list<sha256>::ptr hashes;
    size_t offset = 0;
    string root_hash, meta;
    size_t file_size = 0;
    string ref_meta;

    if (argc != 3) {
      cerr << "usage: " << argv[0] << " <input-prefix> <out-file>" << endl;
      return 1;
    }

    in_prefix = argv[1];

    f_out = fopen(argv[2], "w");
    f_in = fopen((in_prefix + ".s3_out").c_str(), "r");
    f_meta = fopen((in_prefix + ".s3_meta").c_str(), "r");

    if (!f_in || !f_out || !f_meta) {
      cerr << "failed to open input/output file(s)" << endl;
      return 1;
    }

    while (true) {
      char buf[1024];
      string line, prefix;
      size_t pos;

      if (!fgets(buf, 1024, f_meta))
        break;

      buf[strlen(buf) - 1] = '\0'; // strip trailing newline

      line = buf;
      pos = line.find(": ");

      if (pos == string::npos) {
        cerr << "malformed input: " << line << endl;
        return 1;
      }

      prefix = line.substr(0, pos);
      line = line.substr(pos + 2);

      if (prefix == "v_key") {
        v_key = buffer::from_string(line);
      } else if (prefix == "iv") {
        meta_key = symmetric_key::create(v_key, buffer::from_string(line));
      } else if (prefix == "size") {
        file_size = lexical_cast<size_t>(line);
      } else if (prefix == "meta") {
        ref_meta = line;
      } else if (prefix == "meta_enc") {
        string meta = cipher::decrypt<aes_cbc_256_with_pkcs, hex>(meta_key, line);

        if (meta != ref_meta) {
          cerr << "meta mismatch" << endl;
          return 1;
        }

        pos = meta.find('#');

        if (pos == string::npos) {
          cerr << "malformed meta: " << meta << endl;
          return 1;
        }

        file_key = symmetric_key::from_string(meta.substr(0, pos));
        root_hash = meta.substr(pos + 1);
      } else if (prefix == "f_key" || prefix == "root_hash" || prefix == "meta") {
        // ignore
      } else {
        cerr << "unknown prefix: " << prefix << endl;
        return 1;
      }
    }

    hashes.reset(new hash_list<sha256>(file_size));

    while (true) {
      uint8_t buf_in[HASH_BLOCK_SIZE], buf_out[HASH_BLOCK_SIZE];
      size_t read_count = 0;

      read_count = fread(buf_in, 1, HASH_BLOCK_SIZE, f_in);

      aes_ctr_256::decrypt(file_key, buf_in, read_count, buf_out);

      if (fwrite(buf_out, read_count, 1, f_out) != 1) {
        cerr << "failed to write to output file" << endl;
        return 1;
      }

      hashes->compute_hash(offset, buf_out, read_count);
      offset += read_count;

      if (read_count < HASH_BLOCK_SIZE)
        break;
    }

    if (root_hash != hashes->get_root_hash<hex>()) {
      cerr << "hash mismatch" << endl;
      return 1;
    }

    cout << "done" << endl;

    return 0;

  } catch (const std::exception &e) {
    cerr << "caught exception: " << e.what() << endl;
  }

  return 1;
}
