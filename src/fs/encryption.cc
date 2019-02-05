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

namespace s3 {
namespace fs {

namespace {
const int DERIVATION_ROUNDS = 8192;
const int PASSWORD_ATTEMPTS = 5;

crypto::buffer::ptr init_from_file() {
  std::ifstream f;
  std::string key;

  crypto::private_file::open(base::config::get_volume_key_file(), &f);
  getline(f, key);

  return crypto::buffer::from_string(key);
}

crypto::buffer::ptr init_from_password() {
  std::string password;
  std::string prompt;

  prompt = "password for key \"";
  prompt += base::config::get_volume_key_id();
  prompt += "\" in bucket \"";
  prompt += base::config::get_bucket_name();
  prompt += "\": ";

  password = crypto::passwords::read_from_stdin(prompt);

  if (password.empty())
    throw std::runtime_error("cannot use empty password for file encryption.");

  return encryption::derive_key_from_password(password);
}

bucket_volume_key::ptr s_volume_key;
} // namespace

void encryption::init() {
  crypto::buffer::ptr password_key;
  base::request::ptr req;

  if (!base::config::get_use_encryption())
    return;

  if (base::config::get_volume_key_id().empty())
    throw std::runtime_error(
        "volume key id must be set if encryption is enabled.");

  req.reset(new base::request());
  req->set_hook(services::service::get_request_hook());

  s_volume_key =
      bucket_volume_key::fetch(req, base::config::get_volume_key_id());

  if (!s_volume_key)
    throw std::runtime_error(
        "encryption enabled but specified volume key could not be found. check "
        "the configuration and/or run " PACKAGE_NAME "_vol_key.");

  if (base::config::get_volume_key_file().empty()) {
    int retry_count = 0;

    while (true) {
      try {
        s_volume_key->unlock(init_from_password());

        break;

      } catch (...) {
        retry_count++;

        if (retry_count < PASSWORD_ATTEMPTS) {
          std::cout << "incorrect password. please try again." << std::endl;
        } else {
          throw;
        }
      }
    }

  } else {
    s_volume_key->unlock(init_from_file());
  }

  S3_LOG(LOG_DEBUG, "encryption::init", "encryption enabled with id [%s]\n",
         base::config::get_volume_key_id().c_str());
}

crypto::buffer::ptr encryption::get_volume_key() {
  if (!s_volume_key)
    throw std::runtime_error("volume key not available.");

  return s_volume_key->get_volume_key();
}

crypto::buffer::ptr
encryption::derive_key_from_password(const std::string &password) {
  return crypto::pbkdf2_sha1::derive<bucket_volume_key::key_cipher>(
      password, base::config::get_bucket_name(), DERIVATION_ROUNDS);
}

} // namespace fs
} // namespace s3
