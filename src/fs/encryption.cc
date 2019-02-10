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

#include "fs/encryption.h"

#include <iostream>

#include "base/config.h"
#include "base/logger.h"
#include "base/paths.h"
#include "base/request.h"
#include "crypto/buffer.h"
#include "crypto/passwords.h"
#include "crypto/pbkdf2_sha1.h"
#include "crypto/private_file.h"
#include "fs/bucket_volume_key.h"

namespace s3 {
namespace fs {

namespace {
constexpr int DERIVATION_ROUNDS = 8192;
constexpr int PASSWORD_ATTEMPTS = 5;

crypto::Buffer InitFromFile() {
  std::ifstream f;
  crypto::PrivateFile::Open(
      base::Paths::Transform(base::Config::volume_key_file()), &f);

  std::string key;
  std::getline(f, key);

  return crypto::Buffer::FromHexString(key);
}

crypto::Buffer InitFromPassword() {
  std::string prompt = "password for key \"";
  prompt += base::Config::volume_key_id();
  prompt += "\" in bucket \"";
  prompt += base::Config::bucket_name();
  prompt += "\": ";

  std::string password = crypto::Passwords::ReadFromStdin(prompt);
  if (password.empty())
    throw std::runtime_error("cannot use empty password for file encryption.");

  return Encryption::DeriveKeyFromPassword(password);
}

std::unique_ptr<BucketVolumeKey> s_volume_key;
}  // namespace

void Encryption::Init() {
  if (!base::Config::use_encryption()) return;
  if (base::Config::volume_key_id().empty())
    throw std::runtime_error(
        "volume key id must be set if encryption is enabled.");

  auto req = base::RequestFactory::New();
  s_volume_key =
      BucketVolumeKey::Fetch(req.get(), base::Config::volume_key_id());

  if (!s_volume_key)
    throw std::runtime_error(
        "encryption enabled but specified volume key could not be found. check "
        "the configuration and/or run " PACKAGE_NAME "_vol_key.");

  if (base::Config::volume_key_file().empty()) {
    int retry_count = 0;
    while (true) {
      try {
        s_volume_key->Unlock(InitFromPassword());
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
    s_volume_key->Unlock(InitFromFile());
  }

  S3_LOG(LOG_DEBUG, "Encryption::Init", "encryption enabled with id [%s]\n",
         base::Config::volume_key_id().c_str());
}

const crypto::Buffer &Encryption::volume_key() {
  if (!s_volume_key) throw std::runtime_error("volume key not available.");
  return s_volume_key->volume_key();
}

crypto::Buffer Encryption::DeriveKeyFromPassword(const std::string &password) {
  return crypto::Pbkdf2Sha1::Derive<BucketVolumeKey::KeyCipherType>(
      password, base::Config::bucket_name(), DERIVATION_ROUNDS);
}

}  // namespace fs
}  // namespace s3
