/*
 * crypto/keychain.h
 * -------------------------------------------------------------------------
 * macOS Keychain helpers.
 * -------------------------------------------------------------------------
 *
 * Copyright (c) 2019, Tarick Bedeir.
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

#ifndef S3_CRYPTO_KEYCHAIN_H
#define S3_CRYPTO_KEYCHAIN_H

#include <string>

namespace s3 {
namespace crypto {
class Keychain {
 public:
  static std::string BuildIdentifier(const std::string &service,
                                     const std::string &bucket_name,
                                     const std::string &volume_key_id);

  static bool ReadPassword(const std::string &id, std::string *password);
  static void WritePassword(const std::string &id, const std::string &password);
};
}  // namespace crypto
}  // namespace s3

#endif
