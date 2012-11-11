#include "base/config.h"
#include "base/logger.h"
#include "crypto/aes_cbc_256.h"
#include "crypto/buffer.h"
#include "crypto/passwords.h"
#include "crypto/pbkdf2_sha1.h"
#include "crypto/private_file.h"
#include "fs/encryption.h"

using std::ifstream;
using std::runtime_error;
using std::string;

using s3::base::config;
using s3::crypto::aes_cbc_256;
using s3::crypto::buffer;
using s3::crypto::passwords;
using s3::crypto::pbkdf2_sha1;
using s3::crypto::private_file;
using s3::fs::encryption;

namespace
{
  const int DERIVATION_ROUNDS = 8192;

  buffer::ptr init_from_file(const string &key_file)
  {
    ifstream f;
    string key;

    private_file::open(key_file, &f);
    getline(f, key);

    return buffer::from_string(key);
  }

  buffer::ptr init_from_password()
  {
    string password;
    string prompt;

    prompt = "password for bucket \"";
    prompt += config::get_bucket_name();
    prompt += "\": ";

    password = passwords::read_from_stdin(prompt);

    if (password.empty())
      throw runtime_error("cannot use empty password for file encryption");

    return pbkdf2_sha1::derive<aes_cbc_256>(
      password,
      config::get_bucket_name(),
      DERIVATION_ROUNDS);
  }

  buffer::ptr s_volume_key;
}

void encryption::init()
{
  if (!config::get_use_encryption())
    return;

  if (!config::get_volume_key_file().empty())
    s_volume_key = init_from_file(config::get_volume_key_file());
  else
    s_volume_key = init_from_password();

  S3_LOG(
    LOG_DEBUG,
    "encryption::init",
    "encryption enabled\n");
}

buffer::ptr encryption::get_volume_key()
{
  if (!s_volume_key)
    throw runtime_error("volume key not available");

  return s_volume_key;
}
