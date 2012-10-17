#include <fcntl.h>
#include <sys/stat.h>

#include <iostream>

#include "crypto/aes_ctr_256_cipher.h"
#include "crypto/cipher_state.h"

using std::cout;
using std::endl;
using std::runtime_error;
using std::string;

using s3::crypto::aes_ctr_256_cipher;
using s3::crypto::cipher_state;

namespace
{
  const size_t CHUNK_SIZE = 8 * 1024;
}

int run_aes(const cipher_state::ptr &cs, const string &file_in, const string &file_out)
{
  aes_ctr_256_cipher::ptr aes(new aes_ctr_256_cipher(cs, 0));
  int fd_in, fd_out;
  off_t offset = 0;

  cout << "using " << cs->serialize() << endl;

  fd_in = open(file_in.c_str(), O_RDONLY);
  fd_out = open(file_out.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
  
  if (fd_in == -1 || fd_out == -1)
    throw runtime_error("open() failed");

  while (true) {
    uint8_t buf_in[CHUNK_SIZE], buf_out[CHUNK_SIZE];
    ssize_t sz;

    sz = pread(fd_in, buf_in, CHUNK_SIZE, offset);

    if (sz == -1)
      throw runtime_error("pread() failed");

    if (sz == 0)
      break;

    aes->encrypt(buf_in, sz, buf_out);

    sz = pwrite(fd_out, buf_out, sz, offset);

    if (sz <= 0)
      throw runtime_error("pwrite() failed");

    offset += sz;
  }

  close(fd_in);
  close(fd_out);

  cout << "done" << endl;
  return 0;
}

int main(int argc, char **argv)
{
  try {
    if (argc == 3)
      return run_aes(cipher_state::generate<aes_ctr_256_cipher>(), argv[1], argv[2]);
    else if (argc == 4)
      return run_aes(cipher_state::deserialize(argv[1]), argv[2], argv[3]);

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
