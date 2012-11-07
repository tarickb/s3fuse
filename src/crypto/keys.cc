#include "base/logger.h"
#include "crypto/buffer.h"
#include "crypto/keys.h"
#include "crypto/private_file.h"

using std::ifstream;
using std::runtime_error;
using std::string;

using s3::crypto::buffer;
using s3::crypto::keys;
using s3::crypto::private_file;

namespace
{
  buffer::ptr s_volume_key;
}

void keys::init(const string &key_file)
{
  ifstream f;
  string key;

  private_file::open(key_file, &f);
  getline(f, key);

  S3_LOG(LOG_DEBUG, "keys::init", "key: [%s]\n", key.c_str());

  s_volume_key = buffer::from_string(key);
}

buffer::ptr keys::get_volume_key()
{
  if (!s_volume_key)
    throw runtime_error("volume key not available");

  return s_volume_key;
}
