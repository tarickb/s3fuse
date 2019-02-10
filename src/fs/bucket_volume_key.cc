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

#include "fs/bucket_volume_key.h"

#include "base/request.h"
#include "crypto/buffer.h"
#include "crypto/cipher.h"
#include "crypto/symmetric_key.h"
#include "fs/directory.h"
#include "fs/object.h"
#include "services/service.h"

namespace s3 {
namespace fs {

namespace {
const std::string VOLUME_KEY_OBJECT_PREFIX = "encryption_vk_";
const std::string VOLUME_KEY_OBJECT_TEMP_PREFIX = "$temp$_";

const std::string VOLUME_KEY_PREFIX = "s3fuse-00 ";

inline std::string BuildUrl(const std::string &id) {
  return Object::BuildInternalUrl(VOLUME_KEY_OBJECT_PREFIX + id);
}

inline bool IsTemporaryId(const std::string &id) {
  return (id.substr(0, VOLUME_KEY_OBJECT_TEMP_PREFIX.size()) ==
          VOLUME_KEY_OBJECT_TEMP_PREFIX);
}
}  // namespace

std::unique_ptr<BucketVolumeKey> BucketVolumeKey::Fetch(base::Request *req,
                                                        const std::string &id) {
  std::unique_ptr<BucketVolumeKey> key(new BucketVolumeKey(id));
  key->Download(req);
  if (!key->is_present()) key.reset();
  return key;
}

std::unique_ptr<BucketVolumeKey> BucketVolumeKey::Generate(
    base::Request *req, const std::string &id) {
  if (IsTemporaryId(id)) throw std::runtime_error("invalid key id.");
  std::unique_ptr<BucketVolumeKey> key(new BucketVolumeKey(id));
  key->Download(req);
  if (key->is_present())
    throw std::runtime_error("key with specified id already exists.");
  key->Generate();
  return key;
}

std::vector<std::string> BucketVolumeKey::GetKeys(base::Request *req) {
  const size_t PREFIX_LEN = VOLUME_KEY_OBJECT_PREFIX.size();
  std::vector<std::string> keys;
  auto objs = Directory::GetInternalObjects(req);
  for (const auto &obj : objs) {
    if (obj.substr(0, PREFIX_LEN) == VOLUME_KEY_OBJECT_PREFIX) {
      const std::string key = obj.substr(PREFIX_LEN);
      if (!IsTemporaryId(key)) keys.push_back(key);
    }
  }
  return keys;
}

BucketVolumeKey::BucketVolumeKey(const std::string &id)
    : id_(id), volume_key_(crypto::Buffer::Empty()) {}

void BucketVolumeKey::Unlock(const crypto::Buffer &key) {
  if (!is_present())
    throw std::runtime_error("cannot unlock a key that does not exist.");
  std::string bucket_meta;
  try {
    auto sk = crypto::SymmetricKey::Create(
        key, crypto::Buffer::Zero(KeyCipherType::IV_LEN));
    bucket_meta = crypto::Cipher::DecryptAsString<KeyCipherType, crypto::Hex>(
        sk, encrypted_key_);
  } catch (...) {
    throw std::runtime_error("unable to unlock key.");
  }

  if (bucket_meta.substr(0, VOLUME_KEY_PREFIX.size()) != VOLUME_KEY_PREFIX)
    throw std::runtime_error("unable to unlock key.");

  volume_key_ = crypto::Buffer::FromHexString(
      bucket_meta.substr(VOLUME_KEY_PREFIX.size()));
}

void BucketVolumeKey::Remove(base::Request *req) {
  req->Init(base::HttpMethod::DELETE);
  req->SetUrl(BuildUrl(id_));

  req->Run();

  if (req->response_code() != base::HTTP_SC_NO_CONTENT)
    throw std::runtime_error("failed to delete volume key.");
}

std::unique_ptr<BucketVolumeKey> BucketVolumeKey::Clone(
    const std::string &new_id) const {
  if (IsTemporaryId(new_id)) throw std::runtime_error("invalid key id.");
  if (!volume_key_) throw std::runtime_error("unlock key before cloning.");
  std::unique_ptr<BucketVolumeKey> key(new BucketVolumeKey(new_id));
  if (key->is_present())
    throw std::runtime_error("key with specified id already exists.");
  key->volume_key_ = volume_key_;
  return key;
}

void BucketVolumeKey::Commit(base::Request *req, const crypto::Buffer &key) {
  if (!volume_key_) throw std::runtime_error("unlock key before committing.");

  req->Init(base::HttpMethod::PUT);
  req->SetUrl(BuildUrl(VOLUME_KEY_OBJECT_TEMP_PREFIX + id_));
  req->SetInputBuffer(
      crypto::Cipher::Encrypt<BucketVolumeKey::KeyCipherType, crypto::Hex>(
          crypto::SymmetricKey::Create(
              key,
              crypto::Buffer::Zero(BucketVolumeKey::KeyCipherType::IV_LEN)),
          VOLUME_KEY_PREFIX + volume_key_.ToHexString()));

  req->Run();
  if (req->response_code() != base::HTTP_SC_OK)
    throw std::runtime_error(
        "failed to commit (create) volume key; the old "
        "key should remain valid.");

  const std::string etag = req->response_header("ETag");

  req->Init(base::HttpMethod::PUT);
  req->SetUrl(BuildUrl(id_));

  // only overwrite the volume key if our temporary copy is still valid
  req->SetHeader(services::Service::header_prefix() + "copy-source",
                 BuildUrl(VOLUME_KEY_OBJECT_TEMP_PREFIX + id_));
  req->SetHeader(services::Service::header_prefix() + "copy-source-if-match",
                 etag);
  req->SetHeader(services::Service::header_prefix() + "metadata-directive",
                 "REPLACE");

  req->Run();

  if (req->response_code() != base::HTTP_SC_OK)
    throw std::runtime_error(
        "failed to commit (copy) volume key; the old key should remain valid.");

  req->Init(base::HttpMethod::DELETE);
  req->SetUrl(BuildUrl(VOLUME_KEY_OBJECT_TEMP_PREFIX + id_));

  req->Run();
}

void BucketVolumeKey::Download(base::Request *req) {
  req->Init(base::HttpMethod::GET);
  req->SetUrl(BuildUrl(id_));

  req->Run();

  if (req->response_code() == base::HTTP_SC_OK)
    encrypted_key_ = req->GetOutputAsString();
  else if (req->response_code() == base::HTTP_SC_NOT_FOUND)
    encrypted_key_.clear();
  else
    throw std::runtime_error("request for volume key object failed.");
}

void BucketVolumeKey::Generate() {
  volume_key_ = crypto::Buffer::Generate(KeyCipherType::DEFAULT_KEY_LEN);
}

}  // namespace fs
}  // namespace s3
