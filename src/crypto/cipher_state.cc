#ifdef __APPLE__
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

  // TODO: check for error messages, find a darwin way of doing this
  #ifdef __APPLE__
  #else
    RAND_bytes(&_key[0], _key.size());
    RAND_bytes(&_iv[0], _iv.size());
  #endif

  // TODO: explicitly fail on big-endian machines!
}
