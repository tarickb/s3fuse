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

#include <iostream>

#include "base/config.h"
#include "base/logger.h"
#include "base/request.h"
#include "crypto/buffer.h"
#include "crypto/passwords.h"
#include "crypto/pbkdf2_sha1.h"
#include "crypto/private_file.h"
#include "fs/bucket_volume_key.h"
#include "fs/encryption.h"
#include "services/service.h"

using std::cout;
using std::endl;
using std::ifstream;
using std::runtime_error;
using std::string;

using s3::base::config;
using s3::base::request;
using s3::crypto::buffer;
using s3::crypto::passwords;
using s3::crypto::pbkdf2_sha1;
using s3::crypto::private_file;
using s3::fs::bucket_volume_key;
using s3::fs::encryption;
using s3::services::service;

namespace
{
  const int DERIVATION_ROUNDS = 8192;
  const int PASSWORD_ATTEMPTS = 5;

  buffer::ptr init_from_file()
  {
    ifstream f;
    string key;

    private_file::open(config::get_volume_key_file(), &f);
    getline(f, key);

    return buffer::from_string(key);
  }

  buffer::ptr init_from_password()
  {
    string password;
    string prompt;

    prompt = "password for key \"";
    prompt += config::get_volume_key_id();
    prompt += "\" in bucket \"";
    prompt += config::get_bucket_name();
    prompt += "\": ";

    password = passwords::read_from_stdin(prompt);

    if (password.empty())
      throw runtime_error("cannot use empty password for file encryption.");

    return encryption::derive_key_from_password(password);
  }

  bucket_volume_key::ptr s_volume_key;
}

void encryption::init()
{
  buffer::ptr password_key;
  request::ptr req;

  if (!config::get_use_encryption())
    return;

  if (config::get_volume_key_id().empty())
    throw runtime_error("volume key id must be set if encryption is enabled.");

  req.reset(new request());
  req->set_hook(service::get_request_hook());

  s_volume_key = bucket_volume_key::fetch(req, config::get_volume_key_id());

  if (!s_volume_key)
    throw runtime_error("encryption enabled but specified volume key could not be found. check the configuration and/or run s3fuse_vol_key.");

  if (config::get_volume_key_file().empty()) {
    int retry_count = 0;

    while (true) {
      try {
        s_volume_key->unlock(init_from_password());

        break;
      
      } catch (...) {
        retry_count++;

        if (retry_count < PASSWORD_ATTEMPTS) {
          cout << "incorrect password. please try again." << endl;
        } else {
          throw;
        }
      }
    }

  } else {
    s_volume_key->unlock(init_from_file());
  }

  S3_LOG(
    LOG_DEBUG,
    "encryption::init",
    "encryption enabled with id [%s]\n", config::get_volume_key_id().c_str());
}

buffer::ptr encryption::get_volume_key()
{
  if (!s_volume_key)
    throw runtime_error("volume key not available.");

  return s_volume_key->get_volume_key();
}

buffer::ptr encryption::derive_key_from_password(const string &password)
{
  return pbkdf2_sha1::derive<bucket_volume_key::key_cipher>(
    password,
    config::get_bucket_name(),
    DERIVATION_ROUNDS);
}
