/*
 * vol_key.cc
 * -------------------------------------------------------------------------
 * Manages bucket volume keys.
 * -------------------------------------------------------------------------
 *
 * Copyright (c) 2013, Tarick Bedeir.
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

#include <getopt.h>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>

#include "base/config.h"
#include "base/logger.h"
#include "base/paths.h"
#include "base/request.h"
#include "base/xml.h"
#include "crypto/buffer.h"
#include "crypto/passwords.h"
#include "crypto/private_file.h"
#include "fs/bucket_volume_key.h"
#include "fs/encryption.h"
#include "services/service.h"

namespace s3 {
namespace {
constexpr char SHORT_OPTIONS[] = ":c:i:o:";

constexpr option LONG_OPTIONS[] = {
    {"config-file", required_argument, nullptr, 'c'},
    {"in-key", required_argument, nullptr, 'i'},
    {"out-key", required_argument, nullptr, 'o'},
    {nullptr, 0, nullptr, '\0'}};

base::Request *GetRequest() {
  static auto s_request = base::RequestFactory::New();
  return s_request.get();
}

void Init(const std::string &config_file) {
  base::Logger::Init(LOG_ERR);
  base::Config::Init(config_file);
  base::XmlDocument::Init();
  services::Service::Init();
}

void ToLower(std::string *in) {
  std::transform(in->begin(), in->end(), in->begin(),
                 [](char c) { return tolower(c); });
}

void ConfirmKeyDelete(const std::string &key_id) {
  std::cout << "You are going to delete volume encryption key [" << key_id
            << "] "
               "for bucket ["
            << base::Config::bucket_name()
            << "]. Are you sure?\n"
               "Enter \"yes\": ";
  std::string line;
  std::getline(std::cin, line);
  ToLower(&line);
  if (line != "yes") throw std::runtime_error("aborted");
}

void ConfirmLastKeyDelete() {
  std::cout << "You are going to delete the last remaining volume encryption "
               "key for bucket:\n"
               "  "
            << base::Config::bucket_name()
            << "\n"
               "\n"
               "To confirm, enter the name of the bucket (case sensitive): ";
  std::string line;
  std::getline(std::cin, line);
  if (line != base::Config::bucket_name()) throw std::runtime_error("aborted");

  std::cout
      << "\n"
         "*******************************************************************\n"
         "* WARNING                                                         *\n"
         "* -------                                                         *\n"
         "*                                                                 *\n"
         "* What you are about to do will render inaccessible all encrypted *\n"
         "* objects in this bucket.  This operation is irreversible.        *\n"
         "*******************************************************************\n"
         "\n"
         "Do you understand that existing encrypted files will be lost "
         "forever? Type \"yes\": ";
  std::getline(std::cin, line);
  ToLower(&line);
  if (line != "yes") throw std::runtime_error("aborted");

  std::cout << "Do you understand that this operation cannot be undone? Type "
               "\"yes\": ";
  std::getline(std::cin, line);
  ToLower(&line);
  if (line != "yes") throw std::runtime_error("aborted");
}

crypto::Buffer PromptForCurrentPassword(const std::string &key_id) {
  std::string current_password = crypto::Passwords::ReadFromStdin(
      std::string("Enter current password for [") + key_id + "]: ");
  if (current_password.empty())
    throw std::runtime_error("current password not specified");
  return fs::Encryption::DeriveKeyFromPassword(current_password);
}

crypto::Buffer PromptForNewPassword(const std::string &key_id) {
  std::string new_password = crypto::Passwords::ReadFromStdin(
      std::string("Enter new password for [") + key_id + "]: ");
  if (new_password.empty())
    throw std::runtime_error("password cannot be empty");
  std::string confirm = crypto::Passwords::ReadFromStdin(
      std::string("Confirm new password for [") + key_id + "]: ");
  if (confirm != new_password)
    throw std::runtime_error("passwords do not match");
  return fs::Encryption::DeriveKeyFromPassword(new_password);
}

crypto::Buffer ReadKeyFromFile(const std::string &file) {
  std::cout << "Reading key from [" << file << "]..." << std::endl;
  std::ifstream f;
  crypto::PrivateFile::Open(base::Paths::Transform(file), &f);

  std::string line;
  std::getline(f, line);

  return crypto::Buffer::FromHexString(line);
}

crypto::Buffer GenerateAndWrite(const std::string &file) {
  auto key = crypto::Buffer::Generate(
      fs::BucketVolumeKey::KeyCipherType::DEFAULT_KEY_LEN);
  std::cout << "Writing key to [" << file << "]..." << std::endl;
  std::ofstream f;
  crypto::PrivateFile::Open(base::Paths::Transform(file), &f);
  f << key.ToHexString() << std::endl;
  return key;
}

void ListKeys(const std::string &config_file) {
  Init(config_file);
  auto keys = fs::BucketVolumeKey::GetKeys(GetRequest());
  if (keys.empty()) {
    std::cout << "No keys found for bucket [" << base::Config::bucket_name()
              << "]." << std::endl;
    return;
  }
  std::cout << "Keys for bucket [" << base::Config::bucket_name()
            << "]:" << std::endl;
  for (const auto &key : keys) std::cout << "  " << key << std::endl;
}

void GenerateNewKey(const std::string &config_file, const std::string &key_id,
                    const std::string &out_key_file) {
  Init(config_file);
  auto keys = fs::BucketVolumeKey::GetKeys(GetRequest());
  if (!keys.empty())
    throw std::runtime_error(
        "bucket already contains one or more keys. clone an existing key.");
  std::cout << "This bucket does not currently have an encryption key. We'll "
               "create one.\n"
               "\n";
  auto password_key = crypto::Buffer::Empty();
  if (out_key_file.empty())
    password_key = PromptForNewPassword(key_id);
  else
    password_key = GenerateAndWrite(out_key_file);
  std::cout << "Generating volume key [" << key_id << "] for bucket ["
            << base::Config::bucket_name() << "]..." << std::endl;
  auto volume_key = fs::BucketVolumeKey::Generate(GetRequest(), key_id);
  volume_key->Commit(GetRequest(), password_key);
  std::cout << "Done." << std::endl;
}

void CloneKey(const std::string &config_file, const std::string &key_id,
              const std::string &in_key_file, const std::string &new_id,
              const std::string &out_key_file) {
  Init(config_file);
  auto volume_key = fs::BucketVolumeKey::Fetch(GetRequest(), key_id);
  auto new_volume_key = fs::BucketVolumeKey::Fetch(GetRequest(), new_id);
  if (!volume_key) throw std::runtime_error("specified key does not exist.");
  if (new_volume_key)
    throw std::runtime_error(
        "a key already exists with that id. delete it first.");
  auto current_key = crypto::Buffer::Empty();
  if (in_key_file.empty())
    current_key = PromptForCurrentPassword(key_id);
  else
    current_key = ReadKeyFromFile(in_key_file);
  volume_key->Unlock(current_key);
  new_volume_key = volume_key->Clone(new_id);
  auto new_key = crypto::Buffer::Empty();
  if (out_key_file.empty())
    new_key = PromptForCurrentPassword(new_id);
  else
    new_key = GenerateAndWrite(out_key_file);
  std::cout << "Cloning key..." << std::endl;
  new_volume_key->Commit(GetRequest(), new_key);
  std::cout << "Done." << std::endl;
}

void ReEncryptKey(const std::string &config_file, const std::string &key_id,
                  const std::string &in_key_file,
                  const std::string &out_key_file) {
  Init(config_file);
  auto volume_key = fs::BucketVolumeKey::Fetch(GetRequest(), key_id);
  if (!volume_key) throw std::runtime_error("specified key does not exist.");
  auto current_key = crypto::Buffer::Empty();
  if (in_key_file.empty())
    current_key = PromptForCurrentPassword(key_id);
  else
    current_key = ReadKeyFromFile(in_key_file);
  volume_key->Unlock(current_key);
  auto new_key = crypto::Buffer::Empty();
  if (out_key_file.empty())
    new_key = PromptForCurrentPassword(key_id);
  else
    new_key = GenerateAndWrite(out_key_file);
  std::cout << "Changing key..." << std::endl;
  volume_key->Commit(GetRequest(), new_key);
  std::cout << "Done." << std::endl;
}

void DeleteKey(const std::string &config_file, const std::string &key_id) {
  Init(config_file);
  auto volume_key = fs::BucketVolumeKey::Fetch(GetRequest(), key_id);
  if (!volume_key)
    throw std::runtime_error("specified volume key does not exist");
  auto keys = fs::BucketVolumeKey::GetKeys(GetRequest());
  if (keys.size() == 1)
    ConfirmLastKeyDelete();
  else
    ConfirmKeyDelete(key_id);
  std::cout << "Deleting key..." << std::endl;
  volume_key->Remove(GetRequest());
  std::cout << "Done." << std::endl;
}

void PrintUsage(const char *arg0) {
  const char *base_name = std::strrchr(arg0, '/');
  base_name = base_name ? base_name + 1 : arg0;
  std::cerr << "Usage: " << base_name
            << " [options] <command> [...]\n"
               "\n"
               "Where <command> is one of:\n"
               "\n"
               "  list                      List all bucket keys.\n"
               "  generate [key-id]         Generate a new volume key and "
               "write it to the bucket.\n"
               "  change [key-id]           Change the password or key file "
               "used to access the\n"
               "                            volume key stored in the bucket.\n"
               "  clone [key-id] [new-id]   Make a copy of a key.\n"
               "  delete [key-id]           Erase the specified volume key.\n"
               "\n"
               "[options] can be:\n"
               "\n"
               "  -c, --config-file <path>  Use configuration at <path> rather "
               "than the default.\n"
               "  -i, --in-key <path>       Use key at <path> rather than "
               "prompting for the current\n"
               "                            volume password (only valid with "
               "\"change\").\n"
               "  -o, --out-key <path>      Store key at <path> rather than "
               "prompting for a new\n"
               "                            volume password.  The operation "
               "will fail if a file\n"
               "                            exists at this path (only valid "
               "with \"generate\" and\n"
               "                            \"change\").\n"
               "\n"
               "See "
            << base_name << "(1) for examples and a more detailed explanation."
            << std::endl;
  exit(1);
}
}  // namespace
}  // namespace s3

int main(int argc, char **argv) {
  int opt = 0;
  std::string config_file, in_key_file, out_key_file;
  while ((opt = getopt_long(argc, argv, s3::SHORT_OPTIONS, s3::LONG_OPTIONS,
                            nullptr)) != -1) {
    switch (opt) {
      case 'c':
        config_file = optarg;
        break;

      case 'i':
        in_key_file = optarg;
        break;

      case 'o':
        out_key_file = optarg;
        break;

      default:
        s3::PrintUsage(argv[0]);
    }
  }

#define GET_NEXT_ARG(arg)   \
  do {                      \
    if (optind < argc) {    \
      arg = argv[optind++]; \
    }                       \
  } while (0)

  std::string command, key_id, new_id;
  GET_NEXT_ARG(command);
  GET_NEXT_ARG(key_id);
  GET_NEXT_ARG(new_id);

  int ret = 0;
  try {
    if (command == "list") {
      s3::ListKeys(config_file);

    } else if (command == "generate") {
      if (!in_key_file.empty()) s3::PrintUsage(argv[0]);

      if (key_id.empty())
        throw std::runtime_error("need key id to generate a new key.");

      s3::GenerateNewKey(config_file, key_id, out_key_file);

    } else if (command == "clone") {
      if (key_id.empty()) throw std::runtime_error("need existing key id.");

      if (new_id.empty()) throw std::runtime_error("need new key id.");

      s3::CloneKey(config_file, key_id, in_key_file, new_id, out_key_file);

    } else if (command == "change") {
      if (key_id.empty()) throw std::runtime_error("need key id.");

      s3::ReEncryptKey(config_file, key_id, in_key_file, out_key_file);

    } else if (command == "delete") {
      if (!in_key_file.empty() || !out_key_file.empty())
        s3::PrintUsage(argv[0]);

      if (key_id.empty())
        throw std::runtime_error("specify which key id to delete.");

      s3::DeleteKey(config_file, key_id);

    } else {
      s3::PrintUsage(argv[0]);
    }
  } catch (const std::exception &e) {
    std::cout << "Caught exception: " << e.what() << std::endl;
    ret = 1;
  }

  return ret;
}
