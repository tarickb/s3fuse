#include <fstream>
#include <stdexcept>

#include "aes_256_cbc_cipher.h"
#include "cipher_factory.h"
#include "util.h"

using namespace boost;
using namespace std;

using namespace s3;

namespace
{
  cipher::ptr aes_256_cbc_cipher_ctor(const uint8_t *key, const string &iv)
  {
    return cipher::ptr(new aes_256_cbc_cipher(key, iv));
  }

  cipher::ptr null_cipher_ctor(const uint8_t *, const string &)
  {
    return cipher::ptr();
  }
}

cipher_factory::ctor_fn cipher_factory::s_cipher_ctor = null_cipher_ctor;
vector<uint8_t> cipher_factory::s_key;

void cipher_factory::init(const string &cipher, const string &key_file)
{
  ifstream f;

  if (cipher == "aes_256_cbc")
    s_cipher_ctor = aes_256_cbc_cipher_ctor;
  else
    throw runtime_error("unsupported cipher.");

  util::open_private_file(key_file, &f, ios_base::binary);

  f.seekg(0, ios::end);
  s_key.resize(f.tellg());
  f.seekg(0, ios::beg);

  f.read(reinterpret_cast<char *>(&s_key[0]), s_key.size());

  if (static_cast<size_t>(f.gcount()) != s_key.size())
    throw runtime_error("unable to read key.");
}
