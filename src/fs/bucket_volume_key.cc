/*
 * fs/bucket_volume_key.cc
 * -------------------------------------------------------------------------
 * In-bucket volume key management functions.
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

#include "base/request.h"
#include "crypto/buffer.h"
#include "crypto/cipher.h"
#include "crypto/symmetric_key.h"
#include "fs/bucket_volume_key.h"
#include "fs/directory.h"
#include "fs/object.h"
#include "services/service.h"

using std::runtime_error;
using std::string;
using std::vector;

using s3::base::http_method;
using s3::base::request;
using s3::crypto::buffer;
using s3::crypto::cipher;
using s3::crypto::hex;
using s3::crypto::symmetric_key;
using s3::fs::bucket_volume_key;
using s3::fs::directory;
using s3::fs::object;
using s3::services::service;

namespace
{
  const string VOLUME_KEY_OBJECT_PREFIX = "encryption_vk_";
  const string VOLUME_KEY_OBJECT_TEMP_PREFIX = "$temp$_";

  const string VOLUME_KEY_PREFIX = "s3fuse-00 ";

  inline string build_url(const string &id)
  {
    return object::build_internal_url(VOLUME_KEY_OBJECT_PREFIX + id);
  }

  inline bool is_temp_id(const string &id)
  {
    return (id.substr(0, VOLUME_KEY_OBJECT_TEMP_PREFIX.size()) == VOLUME_KEY_OBJECT_TEMP_PREFIX);
  }
}

bucket_volume_key::ptr bucket_volume_key::fetch(const request::ptr &req, const string &id)
{
  bucket_volume_key::ptr key(new bucket_volume_key(id));

  key->download(req);

  if (!key->is_present())
    key.reset();

  return key;
}

bucket_volume_key::ptr bucket_volume_key::generate(const request::ptr &req, const string &id)
{
  bucket_volume_key::ptr key;

  if (is_temp_id(id))
    throw runtime_error("invalid key id.");
  
  key.reset(new bucket_volume_key(id));

  key->download(req);

  if (key->is_present())
    throw runtime_error("key with specified id already exists.");

  key->generate();

  return key;
}

void bucket_volume_key::get_keys(const request::ptr &req, vector<string> *keys)
{
  const size_t PREFIX_LEN = VOLUME_KEY_OBJECT_PREFIX.size();

  vector<string> objs;

  directory::get_internal_objects(req, &objs);

  for (vector<string>::const_iterator itor = objs.begin(); itor != objs.end(); ++itor) {
    if (itor->substr(0, PREFIX_LEN) == VOLUME_KEY_OBJECT_PREFIX) {
      string key = itor->substr(PREFIX_LEN);

      if (!is_temp_id(key))
        keys->push_back(key);
    }
  }
}

bucket_volume_key::bucket_volume_key(const string &id)
  : _id(id)
{
}

void bucket_volume_key::unlock(const buffer::ptr &key)
{
  string bucket_meta;

  if (!is_present())
    throw runtime_error("cannot unlock a key that does not exist.");

  try {
    bucket_meta = cipher::decrypt<key_cipher, hex>(
      symmetric_key::create(key, buffer::zero(key_cipher::IV_LEN)), 
      _encrypted_key);
  } catch (...) {
    throw runtime_error("unable to unlock key.");
  }

  if (bucket_meta.substr(0, VOLUME_KEY_PREFIX.size()) != VOLUME_KEY_PREFIX)
    throw runtime_error("unable to unlock key.");

  _volume_key = buffer::from_string(bucket_meta.substr(VOLUME_KEY_PREFIX.size()));
}

void bucket_volume_key::remove(const request::ptr &req)
{
  req->init(s3::base::HTTP_DELETE);
  req->set_url(build_url(_id));

  req->run();

  if (req->get_response_code() != s3::base::HTTP_SC_NO_CONTENT)
    throw runtime_error("failed to delete volume key.");
}

bucket_volume_key::ptr bucket_volume_key::clone(const string &new_id)
{
  bucket_volume_key::ptr key;

  if (is_temp_id(new_id))
    throw runtime_error("invalid key id.");

  if (!_volume_key)
    throw runtime_error("unlock key before cloning.");
  
  key.reset(new bucket_volume_key(new_id));

  if (key->is_present())
    throw runtime_error("key with specified id already exists.");

  key->_volume_key = _volume_key;

  return key;
}

void bucket_volume_key::commit(const request::ptr &req, const buffer::ptr &key)
{
  string etag;

  if (!_volume_key)
    throw runtime_error("unlock key before committing.");

  req->init(s3::base::HTTP_PUT);
  req->set_url(build_url(VOLUME_KEY_OBJECT_TEMP_PREFIX + _id));

  req->set_input_buffer(
    cipher::encrypt<bucket_volume_key::key_cipher, hex>(
      symmetric_key::create(key, buffer::zero(bucket_volume_key::key_cipher::IV_LEN)),
      VOLUME_KEY_PREFIX + _volume_key->to_string()));

  req->run();

  if (req->get_response_code() != s3::base::HTTP_SC_OK)
    throw runtime_error("failed to commit (create) volume key; the old key should remain valid.");

  etag = req->get_response_header("ETag");

  req->init(s3::base::HTTP_PUT);
  req->set_url(build_url(_id));

  // only overwrite the volume key if our temporary copy is still valid
  req->set_header(service::get_header_prefix() + "copy-source", build_url(VOLUME_KEY_OBJECT_TEMP_PREFIX + _id));
  req->set_header(service::get_header_prefix() + "copy-source-if-match", etag);
  req->set_header(service::get_header_prefix() + "metadata-directive", "REPLACE");

  req->run();

  if (req->get_response_code() != s3::base::HTTP_SC_OK)
    throw runtime_error("failed to commit (copy) volume key; the old key should remain valid.");

  req->init(s3::base::HTTP_DELETE);
  req->set_url(build_url(VOLUME_KEY_OBJECT_TEMP_PREFIX + _id));

  req->run();
}

void bucket_volume_key::download(const request::ptr &req)
{
  req->init(s3::base::HTTP_GET);
  req->set_url(build_url(_id));

  req->run();

  if (req->get_response_code() == s3::base::HTTP_SC_OK)
    _encrypted_key = req->get_output_string();
  else if (req->get_response_code() == s3::base::HTTP_SC_NOT_FOUND)
    _encrypted_key.clear();
  else
    throw runtime_error("request for volume key object failed.");
}

void bucket_volume_key::generate()
{
  _volume_key = buffer::generate(key_cipher::DEFAULT_KEY_LEN);
}
