/*
 * fs/encryption.cc
 * -------------------------------------------------------------------------
 * Filesystem encryption init, key derivation, etc.
 * -------------------------------------------------------------------------
 *
 * Copyright (c) 2012, Tarick Bedeir.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "base/config.h"
#include "base/logger.h"
#include "base/request.h"
#include "crypto/aes_cbc_256.h"
#include "crypto/buffer.h"
#include "crypto/cipher.h"
#include "crypto/passwords.h"
#include "crypto/pbkdf2_sha1.h"
#include "crypto/private_file.h"
#include "crypto/symmetric_key.h"
#include "fs/encryption.h"
#include "fs/object.h"
#include "services/service.h"

using std::ifstream;
using std::runtime_error;
using std::string;

using s3::base::config;
using s3::base::request;
using s3::crypto::aes_cbc_256_with_pkcs;
using s3::crypto::buffer;
using s3::crypto::cipher;
using s3::crypto::hex;
using s3::crypto::passwords;
using s3::crypto::pbkdf2_sha1;
using s3::crypto::private_file;
using s3::crypto::symmetric_key;
using s3::fs::encryption;
using s3::fs::object;
using s3::services::service;

namespace
{
  const int DERIVATION_ROUNDS = 8192;

  const string VOLUME_KEY_OBJECT = "encryption_vk";
  const string VOLUME_KEY_PREFIX = "s3fuse-00 ";

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

    return pbkdf2_sha1::derive<aes_cbc_256_with_pkcs>(
      password,
      config::get_bucket_name(),
      DERIVATION_ROUNDS);
  }

  buffer::ptr read_volume_key_from_bucket(const buffer::ptr &password_key)
  {
    string bucket_meta;
    request req;

    req.init(s3::base::HTTP_GET);
    req.set_hook(service::get_request_hook());
    req.set_url(object::build_internal_url(VOLUME_KEY_OBJECT));
    req.run();

    if (req.get_response_code() == s3::base::HTTP_SC_NOT_FOUND)
      throw runtime_error("bucket does not contain an encryption key. create one with s3fuse_gen_key.");

    if (req.get_response_code() != s3::base::HTTP_SC_OK)
      throw runtime_error("error while fetching bucket encryption key. try re-creating one with s3fuse_gen_key.");

    bucket_meta = cipher::decrypt<aes_cbc_256_with_pkcs, hex>(
      symmetric_key::create(password_key, buffer::zero(aes_cbc_256_with_pkcs::IV_LEN)), 
      req.get_output_string());

    if (bucket_meta.substr(0, VOLUME_KEY_PREFIX.size()) != VOLUME_KEY_PREFIX)
      throw runtime_error("incorrect password.");

    return buffer::from_string(bucket_meta.substr(VOLUME_KEY_PREFIX.size()));
  }

  buffer::ptr s_volume_key;
}

void encryption::init()
{
  buffer::ptr password_key;

  if (!config::get_use_encryption())
    return;

  if (!config::get_volume_key_file().empty())
    password_key = init_from_file(config::get_volume_key_file());
  else
    password_key = init_from_password();

  s_volume_key = read_volume_key_from_bucket(password_key);

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
