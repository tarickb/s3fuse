/*
 * fs/encrypted_file.cc
 * -------------------------------------------------------------------------
 * Encrypted file class implementation.
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

#include "fs/encrypted_file.h"

#include <atomic>

#include "base/logger.h"
#include "base/request.h"
#include "base/statistics.h"
#include "crypto/aes_cbc_256.h"
#include "crypto/aes_ctr_256.h"
#include "crypto/cipher.h"
#include "crypto/hex.h"
#include "crypto/symmetric_key.h"
#include "fs/encryption.h"
#include "fs/metadata.h"
#include "services/service.h"

namespace s3 {
namespace fs {

namespace {
constexpr char CONTENT_TYPE[] =
    "binary/encrypted-s3fuse-file_0100";  // version 1.0
const std::string META_VERIFIER = "s3fuse_enc_meta ";

std::atomic_int s_non_empty_but_not_intact(0), s_no_iv_or_meta(0),
    s_init_errors(0), s_open_without_key(0);

Object *Checker(const std::string &path, base::Request *req) {
  if (req->response_header("Content-Type") != CONTENT_TYPE) return nullptr;
  return new EncryptedFile(path);
}

void StatsWriter(std::ostream *o) {
  *o << "encrypted files:\n"
        "  non-empty file that isn't intact: "
     << s_non_empty_but_not_intact
     << "\n"
        "  init without iv or metadata: "
     << s_no_iv_or_meta
     << "\n"
        "  init errors: "
     << s_init_errors
     << "\n"
        "  open without key: "
     << s_open_without_key << "\n";
}

Object::TypeCheckers::Entry s_checker_reg(Checker, 100);
base::Statistics::Writers::Entry s_writer(StatsWriter, 0);
}  // namespace

EncryptedFile::EncryptedFile(const std::string &path) : File(path) {
  set_content_type(CONTENT_TYPE);
}

void EncryptedFile::Init(base::Request *req) {
  File::Init(req);

  // there are two cases where we'll encounter an encrypted file that isn't
  // intact, or has no IV and/or no metadata:
  //
  // 1. the file is new, has size zero, and has yet to get metadata or an IV.
  //
  // 2. the file was new, content was written, the file was flushed/uploaded,
  //    but the commit() that would have saved the IV and metadata failed.
  //    there's no point trying to do anything with a file like this -- the
  //    decryption key has been lost.
  //
  // since case #2 results in a useless file, we force the file size to zero.
  // forcing the size to zero has no effect in case #1 because a new file
  // always has size == 0.

  if (!intact()) {
    if (stat()->st_size > 0) {
      S3_LOG(LOG_DEBUG, "EncryptedFile::Init", "file [%s] is not intact\n",
             path().c_str());
      stat()->st_size = 0;
      ++s_non_empty_but_not_intact;
    }
    return;
  }

  const std::string meta_prefix = services::Service::header_meta_prefix();
  enc_iv_ = req->response_header(meta_prefix + Metadata::ENC_IV);
  enc_meta_ = req->response_header(meta_prefix + Metadata::ENC_METADATA);

  if (enc_iv_.empty() || enc_meta_.empty()) {
    S3_LOG(LOG_DEBUG, "EncryptedFile::Init", "file [%s] has no IV/metadata\n",
           path().c_str());
    stat()->st_size = 0;
    ++s_no_iv_or_meta;
    return;
  }

  try {
    meta_key_ = crypto::SymmetricKey::Create(
        Encryption::volume_key(), crypto::Buffer::FromHexString(enc_iv_));

    std::string meta;
    try {
      meta = crypto::Cipher::DecryptAsString<crypto::AesCbc256WithPkcs,
                                             crypto::Hex>(meta_key_, enc_meta_);
    } catch (...) {
      throw std::runtime_error(
          "failed to decrypt file metadata. this probably "
          "means the volume key is invalid.");
    }
    if (meta.substr(0, META_VERIFIER.size()) != META_VERIFIER)
      throw std::runtime_error(
          "file metadata not valid. this probably means "
          "the volume key is invalid.");
    meta = meta.substr(META_VERIFIER.size());
    size_t pos = meta.find('#');
    if (pos == std::string::npos)
      throw std::runtime_error("malformed encrypted file metadata");
    data_key_ = crypto::SymmetricKey::FromString(meta.substr(0, pos));

    SetSha256Hash(meta.substr(pos + 1));

  } catch (const std::exception &e) {
    ++s_init_errors;
    S3_LOG(LOG_WARNING, "EncryptedFile::init",
           "caught exception while initializing [%s]: %s\n", path().c_str(),
           e.what());
    meta_key_ = crypto::SymmetricKey::Empty();
    data_key_ = crypto::SymmetricKey::Empty();
  }

  // by not throwing an exception when something goes wrong here, we leave a
  // usable object that can be renamed/moved/etc. but that cannot be opened.
}

void EncryptedFile::SetRequestHeaders(base::Request *req) {
  File::SetRequestHeaders(req);

  const std::string meta_prefix = services::Service::header_meta_prefix();
  // hide the real hash
  req->SetHeader(meta_prefix + Metadata::SHA256, "");
  req->SetHeader(meta_prefix + Metadata::ENC_IV, enc_iv_);
  req->SetHeader(meta_prefix + Metadata::ENC_METADATA, enc_meta_);
}

int EncryptedFile::IsDownloadable() {
  if (!data_key_) {
    ++s_open_without_key;
    S3_LOG(LOG_WARNING, "EncryptedFile::is_downloadable",
           "cannot open [%s] without key\n", path().c_str());
    return -EACCES;
  }
  return 0;
}

int EncryptedFile::ReadChunk(size_t size, off_t offset,
                             std::vector<char> *buffer) {
  std::vector<char> temp;
  int r = File::ReadChunk(size, offset, &temp);
  if (r) return r;

  buffer->resize(temp.size());
  crypto::AesCtr256::EncryptWithByteOffset(
      data_key_, offset, reinterpret_cast<const uint8_t *>(&temp[0]),
      temp.size(), reinterpret_cast<uint8_t *>(&(*buffer)[0]));
  return 0;
}

int EncryptedFile::WriteChunk(const char *buffer, size_t size, off_t offset) {
  std::vector<char> temp(size);
  crypto::AesCtr256::DecryptWithByteOffset(
      data_key_, offset, reinterpret_cast<const uint8_t *>(buffer), size,
      reinterpret_cast<uint8_t *>(&temp[0]));
  return File::WriteChunk(&temp[0], size, offset);
}

int EncryptedFile::PrepareUpload() {
  meta_key_ = crypto::SymmetricKey::Generate<crypto::AesCbc256WithPkcs>(
      Encryption::volume_key());
  data_key_ = crypto::SymmetricKey::Generate<crypto::AesCtr256>();

  enc_iv_.clear();
  enc_meta_.clear();

  return File::PrepareUpload();
}

int EncryptedFile::FinalizeUpload(const std::string &returned_etag) {
  int r = File::FinalizeUpload(returned_etag);
  if (r) return r;
  enc_iv_ = meta_key_.iv().ToHexString();
  enc_meta_ = crypto::Cipher::Encrypt<crypto::AesCbc256WithPkcs, crypto::Hex>(
      meta_key_, META_VERIFIER + data_key_.ToString() + "#" + sha256_hash());
  return 0;
}

}  // namespace fs
}  // namespace s3
