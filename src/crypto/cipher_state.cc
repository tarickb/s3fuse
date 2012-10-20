#ifdef __APPLE__
  #include <stdio.h>
#else
  #include <openssl/rand.h>
#endif

#include <stdexcept>

#include "crypto/cipher_state.h"
#include "crypto/encoder.h"
#include "crypto/hex.h"

using std::runtime_error;
using std::string;

using s3::crypto::cipher_state;
using s3::crypto::encoder;
using s3::crypto::hex;

cipher_state::ptr cipher_state::deserialize(const string &state)
{
  ptr cs(new cipher_state());
  size_t pos;

  pos = state.find(':');

  if (pos == string::npos)
    throw runtime_error("malformed cipher state string");

  encoder::decode<hex>(state.substr(0, pos), &cs->_key);
  encoder::decode<hex>(state.substr(pos + 1), &cs->_iv);

  return cs;
}

string cipher_state::serialize()
{
  return 
    encoder::encode<hex>(&_key[0], _key.size()) + ":" +
    encoder::encode<hex>(&_iv[0], _iv.size());
}

void cipher_state::generate(size_t key_len, size_t iv_len)
{
  _key.resize(key_len);
  _iv.resize(iv_len);

  // TODO: explicitly fail on big-endian machines!

  #ifdef __APPLE__
    FILE *fp = fopen("/dev/random", "r");
    size_t r = 0;

    if (!fp)
      throw runtime_error("cannot open /dev/random");

    r += fread(&_key[0], _key.size(), 1, fp);
    r += fread(&_iv[0], _iv.size(), 1, fp);

    fclose(fp);

    if (r != 2) // r counts items, not bytes
      throw runtime_error("failed to read from /dev/random");
  #else
    if (RAND_bytes(&_key[0], _key.size()) == 0)
      throw runtime_error("failed to generate random key");

    if (RAND_bytes(&_iv[0], _iv.size()) == 0)
      throw runtime_error("failed to generate random IV");
  #endif
}
