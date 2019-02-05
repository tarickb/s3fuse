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

#include <cstring>
#include <fstream>
#include <iostream>

#include "base/config.h"
#include "crypto/buffer.h"
#include "crypto/passwords.h"
#include "crypto/private_file.h"
#include "fs/bucket_volume_key.h"
#include "fs/encryption.h"
#include "init.h"
#include "services/aws/impl.h"
#include "services/gs/impl.h"
#include "services/service.h"

namespace {
typedef std::vector<std::string> string_vector;

const char *SHORT_OPTIONS = ":c:i:o:";

const option LONG_OPTIONS[] = {{"config-file", required_argument, NULL, 'c'},
                               {"in-key", required_argument, NULL, 'i'},
                               {"out-key", required_argument, NULL, 'o'},
                               {NULL, 0, NULL, '\0'}};

s3::base::request::ptr s_request;

const s3::base::request::ptr &get_request() {
  if (!s_request) {
    s_request.reset(new s3::base::request());

    s_request->set_hook(s3::services::service::get_request_hook());
  }

  return s_request;
}
} // namespace

void init(const std::string &config_file) {
  s3::init::base(s3::init::IB_NONE, LOG_ERR, config_file);
  s3::init::services();
}

std::string to_lower(std::string in) {
  std::string out(in);

  for (std::string::iterator itor = out.begin(); itor != out.end(); ++itor)
    *itor = tolower(*itor);

  return out;
}

void confirm_key_delete(const std::string &key_id) {
  std::string line;

  std::cout << "You are going to delete volume encryption key [" << key_id
            << "] "
               "for bucket ["
            << s3::base::config::get_bucket_name()
            << "]. Are you sure?\n"
               "Enter \"yes\": ";

  std::getline(std::cin, line);

  if (to_lower(line) != "yes")
    throw std::runtime_error("aborted");
}

void confirm_last_key_delete() {
  std::string line;

  std::cout << "You are going to delete the last remaining volume encryption "
               "key for bucket:\n"
               "  "
            << s3::base::config::get_bucket_name()
            << "\n"
               "\n"
               "To confirm, enter the name of the bucket (case sensitive): ";

  std::getline(std::cin, line);

  if (line != s3::base::config::get_bucket_name())
    throw std::runtime_error("aborted");

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
  line = to_lower(line);

  if (line != "yes")
    throw std::runtime_error("aborted");

  std::cout << "Do you understand that this operation cannot be undone? Type "
               "\"yes\": ";

  std::getline(std::cin, line);
  line = to_lower(line);

  if (line != "yes")
    throw std::runtime_error("aborted");
}

s3::crypto::buffer::ptr prompt_for_current_password(const std::string &key_id) {
  std::string current_password;

  current_password = s3::crypto::passwords::read_from_stdin(
      std::string("Enter current password for [") + key_id + "]: ");

  if (current_password.empty())
    throw std::runtime_error("current password not specified");

  return s3::fs::encryption::derive_key_from_password(current_password);
}

s3::crypto::buffer::ptr prompt_for_new_password(const std::string &key_id) {
  std::string new_password, confirm;

  new_password = s3::crypto::passwords::read_from_stdin(
      std::string("Enter new password for [") + key_id + "]: ");

  if (new_password.empty())
    throw std::runtime_error("password cannot be empty");

  confirm = s3::crypto::passwords::read_from_stdin(
      std::string("Confirm new password for [") + key_id + "]: ");

  if (confirm != new_password)
    throw std::runtime_error("passwords do not match");

  return s3::fs::encryption::derive_key_from_password(new_password);
}

s3::crypto::buffer::ptr read_key_from_file(const std::string &file) {
  std::ifstream f;
  std::string line;

  std::cout << "Reading key from [" << file << "]..." << std::endl;
  s3::crypto::private_file::open(file, &f);
  std::getline(f, line);

  return s3::crypto::buffer::from_string(line);
}

s3::crypto::buffer::ptr generate_and_write(const std::string &file) {
  std::ofstream f;
  s3::crypto::buffer::ptr key;

  key = s3::crypto::buffer::generate(
      s3::fs::bucket_volume_key::key_cipher::DEFAULT_KEY_LEN);

  std::cout << "Writing key to [" << file << "]..." << std::endl;
  s3::crypto::private_file::open(file, &f);

  f << key->to_string() << std::endl;

  return key;
}

void list_keys(const std::string &config_file) {
  string_vector keys;

  init(config_file);

  s3::fs::bucket_volume_key::get_keys(get_request(), &keys);

  if (keys.empty()) {
    std::cout << "No keys found for bucket ["
              << s3::base::config::get_bucket_name() << "]." << std::endl;
    return;
  }

  std::cout << "Keys for bucket [" << s3::base::config::get_bucket_name()
            << "]:" << std::endl;

  for (string_vector::const_iterator itor = keys.begin(); itor != keys.end();
       ++itor)
    std::cout << "  " << *itor << std::endl;
}

void generate_new_key(const std::string &config_file, const std::string &key_id,
                      const std::string &out_key_file) {
  s3::crypto::buffer::ptr password_key;
  string_vector keys;
  s3::fs::bucket_volume_key::ptr volume_key;

  init(config_file);

  s3::fs::bucket_volume_key::get_keys(get_request(), &keys);

  if (!keys.empty())
    throw std::runtime_error(
        "bucket already contains one or more keys. clone an existing key.");

  std::cout << "This bucket does not currently have an encryption key. We'll "
               "create one.\n"
               "\n";

  if (out_key_file.empty())
    password_key = prompt_for_new_password(key_id);
  else
    password_key = generate_and_write(out_key_file);

  std::cout << "Generating volume key [" << key_id << "] for bucket ["
            << s3::base::config::get_bucket_name() << "]..." << std::endl;

  volume_key = s3::fs::bucket_volume_key::generate(get_request(), key_id);
  volume_key->commit(get_request(), password_key);

  std::cout << "Done." << std::endl;
}

void clone_key(const std::string &config_file, const std::string &key_id,
               const std::string &in_key_file, const std::string &new_id,
               const std::string &out_key_file) {
  s3::crypto::buffer::ptr current_key, new_key;
  s3::fs::bucket_volume_key::ptr volume_key, new_volume_key;

  init(config_file);

  volume_key = s3::fs::bucket_volume_key::fetch(get_request(), key_id);
  new_volume_key = s3::fs::bucket_volume_key::fetch(get_request(), new_id);

  if (!volume_key)
    throw std::runtime_error("specified key does not exist.");

  if (new_volume_key)
    throw std::runtime_error(
        "a key already exists with that id. delete it first.");

  if (in_key_file.empty())
    current_key = prompt_for_current_password(key_id);
  else
    current_key = read_key_from_file(in_key_file);

  volume_key->unlock(current_key);
  new_volume_key = volume_key->clone(new_id);

  if (out_key_file.empty())
    new_key = prompt_for_new_password(new_id);
  else
    new_key = generate_and_write(out_key_file);

  std::cout << "Cloning key..." << std::endl;
  new_volume_key->commit(get_request(), new_key);

  std::cout << "Done." << std::endl;
}

void reencrypt_key(const std::string &config_file, const std::string &key_id,
                   const std::string &in_key_file,
                   const std::string &out_key_file) {
  s3::crypto::buffer::ptr current_key, new_key;
  s3::fs::bucket_volume_key::ptr volume_key;

  init(config_file);

  volume_key = s3::fs::bucket_volume_key::fetch(get_request(), key_id);

  if (!volume_key)
    throw std::runtime_error("specified key does not exist.");

  if (in_key_file.empty())
    current_key = prompt_for_current_password(key_id);
  else
    current_key = read_key_from_file(in_key_file);

  volume_key->unlock(current_key);

  if (out_key_file.empty())
    new_key = prompt_for_new_password(key_id);
  else
    new_key = generate_and_write(out_key_file);

  std::cout << "Changing key..." << std::endl;
  volume_key->commit(get_request(), new_key);

  std::cout << "Done." << std::endl;
}

void delete_key(const std::string &config_file, const std::string &key_id) {
  s3::fs::bucket_volume_key::ptr volume_key;
  string_vector keys;

  init(config_file);

  volume_key = s3::fs::bucket_volume_key::fetch(get_request(), key_id);

  if (!volume_key)
    throw std::runtime_error("specified volume key does not exist");

  s3::fs::bucket_volume_key::get_keys(get_request(), &keys);

  if (keys.size() == 1)
    confirm_last_key_delete();
  else
    confirm_key_delete(key_id);

  std::cout << "Deleting key..." << std::endl;
  volume_key->remove(get_request());

  std::cout << "Done." << std::endl;
}

void print_usage(const char *arg0) {
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

int main(int argc, char **argv) {
  int opt = 0, ret = 0;
  std::string config_file, in_key_file, out_key_file;
  std::string command, key_id, new_id;

  while ((opt = getopt_long(argc, argv, SHORT_OPTIONS, LONG_OPTIONS, NULL)) !=
         -1) {
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
      print_usage(argv[0]);
    }
  }

#define GET_NEXT_ARG(arg)                                                      \
  do {                                                                         \
    if (optind < argc) {                                                       \
      arg = argv[optind++];                                                    \
    }                                                                          \
  } while (0)

  GET_NEXT_ARG(command);
  GET_NEXT_ARG(key_id);
  GET_NEXT_ARG(new_id);

  try {
    if (command == "list") {
      list_keys(config_file);

    } else if (command == "generate") {
      if (!in_key_file.empty())
        print_usage(argv[0]);

      if (key_id.empty())
        throw std::runtime_error("need key id to generate a new key.");

      generate_new_key(config_file, key_id, out_key_file);

    } else if (command == "clone") {
      if (key_id.empty())
        throw std::runtime_error("need existing key id.");

      if (new_id.empty())
        throw std::runtime_error("need new key id.");

      clone_key(config_file, key_id, in_key_file, new_id, out_key_file);

    } else if (command == "change") {
      if (key_id.empty())
        throw std::runtime_error("need key id.");

      reencrypt_key(config_file, key_id, in_key_file, out_key_file);

    } else if (command == "delete") {
      if (!in_key_file.empty() || !out_key_file.empty())
        print_usage(argv[0]);

      if (key_id.empty())
        throw std::runtime_error("specify which key id to delete.");

      delete_key(config_file, key_id);

    } else {
      print_usage(argv[0]);
    }

  } catch (const std::exception &e) {
    std::cout << "Caught exception: " << e.what() << std::endl;
    ret = 1;
  }

  // do this here because request's dtor has dependencies on other static
  // variables
  s_request.reset();

  return ret;
}
