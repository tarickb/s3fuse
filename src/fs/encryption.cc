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
using s3::base::http_method;
using s3::base::request;
using s3::crypto::aes_cbc_256;
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
  const string VOLUME_KEY_OBJECT_TEMP = "encryption_vk_temp";

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
      throw runtime_error("cannot use empty password for file encryption.");

    return encryption::derive_key_from_password(password);
  }

  request::ptr build_request(http_method method, const string &object = VOLUME_KEY_OBJECT)
  {
    request::ptr req(new request());

    req->init(method);
    req->set_hook(service::get_request_hook());
    req->set_url(object::build_internal_url(object));

    return req;
  }

  buffer::ptr read_volume_key_from_bucket(const buffer::ptr &password_key)
  {
    string bucket_meta;
    request::ptr req = build_request(s3::base::HTTP_GET);

    req->run();

    if (req->get_response_code() == s3::base::HTTP_SC_NOT_FOUND)
      throw runtime_error("bucket does not contain an encryption key. create one with s3fuse_gen_key.");

    if (req->get_response_code() != s3::base::HTTP_SC_OK)
      throw runtime_error("error while fetching bucket encryption key. try re-creating one with s3fuse_gen_key.");

    try {
      bucket_meta = cipher::decrypt<aes_cbc_256_with_pkcs, hex>(
        symmetric_key::create(password_key, buffer::zero(aes_cbc_256::IV_LEN)), 
        req->get_output_string());
    } catch (...) {
      throw runtime_error("incorrect password.");
    }

    if (bucket_meta.substr(0, VOLUME_KEY_PREFIX.size()) != VOLUME_KEY_PREFIX)
      throw runtime_error("incorrect password.");

    return buffer::from_string(bucket_meta.substr(VOLUME_KEY_PREFIX.size()));
  }

  string build_bucket_meta(const buffer::ptr &volume_key, const buffer::ptr &password_key)
  {
    string bucket_meta;

    bucket_meta = cipher::encrypt<aes_cbc_256_with_pkcs, hex>(
      symmetric_key::create(password_key, buffer::zero(aes_cbc_256::IV_LEN)),
      VOLUME_KEY_PREFIX + volume_key->to_string());

    return bucket_meta;
  }

  buffer::ptr s_volume_key;
}

void encryption::init()
{
  buffer::ptr password_key;

  if (!config::get_use_encryption())
    return;

  if (!encryption::is_volume_key_present_in_bucket())
    throw runtime_error("encryption enabled but bucket has no key. run s3fuse_gen_key.");

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
    throw runtime_error("volume key not available.");

  return s_volume_key;
}

buffer::ptr encryption::derive_key_from_password(const string &password)
{
  return pbkdf2_sha1::derive<aes_cbc_256>(
    password,
    config::get_bucket_name(),
    DERIVATION_ROUNDS);
}

bool encryption::is_volume_key_present_in_bucket()
{
  request::ptr req = build_request(s3::base::HTTP_HEAD);

  req->run();

  if (req->get_response_code() == s3::base::HTTP_SC_OK)
    return true;

  if (req->get_response_code() == s3::base::HTTP_SC_NOT_FOUND)
    return false;

  throw runtime_error("request for volume key object failed.");
}

void encryption::write_new_volume_key_to_bucket(const buffer::ptr &key)
{
  buffer::ptr volume_key;
  request::ptr req;

  volume_key = buffer::generate(aes_cbc_256::DEFAULT_KEY_LEN);
  req = build_request(s3::base::HTTP_PUT);

  req->set_input_buffer(build_bucket_meta(volume_key, key));
  req->run();

  if (req->get_response_code() != s3::base::HTTP_SC_OK)
    throw runtime_error("failed to store volume key.");
}

void encryption::reencrypt_volume_key_in_bucket(const buffer::ptr &old_key, const buffer::ptr &new_key)
{
  buffer::ptr volume_key;
  request::ptr req;
  string etag;

  volume_key = read_volume_key_from_bucket(old_key);

  req = build_request(s3::base::HTTP_PUT, VOLUME_KEY_OBJECT_TEMP);

  req->set_input_buffer(build_bucket_meta(volume_key, new_key));
  req->run();

  if (req->get_response_code() != s3::base::HTTP_SC_OK)
    throw runtime_error("failed to reencrypt volume key; the old key should remain valid.");

  etag = req->get_response_header("ETag");

  req = build_request(s3::base::HTTP_PUT);

  // only overwrite the volume key if our temporary copy is still valid
  req->set_header(service::get_header_prefix() + "copy-source", object::build_internal_url(VOLUME_KEY_OBJECT_TEMP));
  req->set_header(service::get_header_prefix() + "copy-source-if-match", etag);
  req->set_header(service::get_header_prefix() + "metadata-directive", "REPLACE");

  req->run();

  if (req->get_response_code() != s3::base::HTTP_SC_OK)
    throw runtime_error("failed to reencrypt volume key; the old key should remain valid.");

  req = build_request(s3::base::HTTP_DELETE, VOLUME_KEY_OBJECT_TEMP);

  req->run();
}

void encryption::delete_volume_key_from_bucket()
{
  request req;

  req.init(s3::base::HTTP_DELETE);
  req.set_hook(service::get_request_hook());
  req.set_url(object::build_internal_url(VOLUME_KEY_OBJECT));
  req.run();

  if (req.get_response_code() != s3::base::HTTP_SC_NO_CONTENT)
    throw runtime_error("failed to delete volume key.");
}
