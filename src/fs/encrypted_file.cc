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

#include <atomic>

#include "base/logger.h"
#include "base/request.h"
#include "base/statistics.h"
#include "crypto/aes_cbc_256.h"
#include "crypto/aes_ctr_256.h"
#include "crypto/cipher.h"
#include "crypto/hex.h"
#include "crypto/symmetric_key.h"
#include "fs/encrypted_file.h"
#include "fs/encryption.h"
#include "fs/metadata.h"
#include "services/service.h"

namespace s3 {
namespace fs {

namespace {
const std::string CONTENT_TYPE =
    "binary/encrypted-s3fuse-file_0100"; // version 1.0
const std::string META_VERIFIER = "s3fuse_enc_meta ";

std::atomic_int s_non_empty_but_not_intact(0), s_no_iv_or_meta(0),
    s_init_errors(0), s_open_without_key(0);

object *checker(const std::string &path, const base::request::ptr &req) {
  if (req->get_response_header("Content-Type") != CONTENT_TYPE)
    return NULL;

  return new encrypted_file(path);
}

void statistics_writer(std::ostream *o) {
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

object::type_checker_list::entry s_checker_reg(checker, 100);
base::statistics::writers::entry s_writer(statistics_writer, 0);
} // namespace

encrypted_file::encrypted_file(const std::string &path) : file(path) {
  set_content_type(CONTENT_TYPE);
}

encrypted_file::~encrypted_file() {}

void encrypted_file::init(const base::request::ptr &req) {
  const std::string &meta_prefix = services::service::get_header_meta_prefix();
  std::string meta;

  file::init(req);

  // there are two cases where we'll encounter an encrypted_file that isn't
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

  if (!is_intact()) {
    if (get_stat()->st_size > 0) {
      S3_LOG(LOG_DEBUG, "encrypted_file::init", "file [%s] is not intact\n",
             get_path().c_str());

      force_zero_size();
      ++s_non_empty_but_not_intact;
    }

    return;
  }

  _enc_iv = req->get_response_header(meta_prefix + metadata::ENC_IV);
  _enc_meta = req->get_response_header(meta_prefix + metadata::ENC_METADATA);

  if (_enc_iv.empty() || _enc_meta.empty()) {
    S3_LOG(LOG_DEBUG, "encrypted_file::init", "file [%s] has no IV/metadata\n",
           get_path().c_str());

    force_zero_size();
    ++s_no_iv_or_meta;

    return;
  }

  try {
    size_t pos;

    _meta_key = crypto::symmetric_key::create(
        encryption::get_volume_key(), crypto::buffer::from_string(_enc_iv));

    try {
      meta =
          crypto::cipher::decrypt<crypto::aes_cbc_256_with_pkcs, crypto::hex>(
              _meta_key, _enc_meta);
    } catch (...) {
      throw std::runtime_error("failed to decrypt file metadata. this probably "
                               "means the volume key is invalid.");
    }

    if (meta.substr(0, META_VERIFIER.size()) != META_VERIFIER)
      throw std::runtime_error("file metadata not valid. this probably means "
                               "the volume key is invalid.");

    meta = meta.substr(META_VERIFIER.size());
    pos = meta.find('#');

    if (pos == std::string::npos)
      throw std::runtime_error("malformed encrypted file metadata");

    _data_key = crypto::symmetric_key::from_string(meta.substr(0, pos));

    set_sha256_hash(meta.substr(pos + 1));

  } catch (const std::exception &e) {
    ++s_init_errors;

    S3_LOG(LOG_WARNING, "encrypted_file::init",
           "caught exception while initializing [%s]: %s\n", get_path().c_str(),
           e.what());

    _meta_key.reset();
    _data_key.reset();
  }

  // by not throwing an exception when something goes wrong here, we leave a
  // usable object that can be renamed/moved/etc. but that cannot be opened.
}

void encrypted_file::set_request_headers(const base::request::ptr &req) {
  const std::string &meta_prefix = services::service::get_header_meta_prefix();

  file::set_request_headers(req);

  // hide the real hash
  req->set_header(meta_prefix + metadata::SHA256, "");

  req->set_header(meta_prefix + metadata::ENC_IV, _enc_iv);
  req->set_header(meta_prefix + metadata::ENC_METADATA, _enc_meta);
}

int encrypted_file::is_downloadable() {
  if (!_data_key) {
    ++s_open_without_key;

    S3_LOG(LOG_WARNING, "encrypted_file::is_downloadable",
           "cannot open [%s] without key\n", get_path().c_str());

    return -EACCES;
  }

  return 0;
}

int encrypted_file::prepare_upload() {
  _meta_key = crypto::symmetric_key::generate<crypto::aes_cbc_256_with_pkcs>(
      encryption::get_volume_key());
  _data_key = crypto::symmetric_key::generate<crypto::aes_ctr_256>();

  _enc_iv.clear();
  _enc_meta.clear();

  return file::prepare_upload();
}

int encrypted_file::finalize_upload(const std::string &returned_etag) {
  int r;

  r = file::finalize_upload(returned_etag);

  if (r)
    return r;

  _enc_iv = _meta_key->get_iv()->to_string();
  _enc_meta =
      crypto::cipher::encrypt<crypto::aes_cbc_256_with_pkcs, crypto::hex>(
          _meta_key,
          META_VERIFIER + _data_key->to_string() + "#" + get_sha256_hash());

  return 0;
}

int encrypted_file::read_chunk(size_t size, off_t offset,
                               const base::char_vector_ptr &buffer) {
  base::char_vector_ptr temp(new base::char_vector());
  int r;

  r = file::read_chunk(size, offset, temp);

  if (r)
    return r;

  buffer->resize(temp->size());

  crypto::aes_ctr_256::encrypt_with_byte_offset(
      _data_key, offset, reinterpret_cast<const uint8_t *>(&(*temp)[0]),
      temp->size(), reinterpret_cast<uint8_t *>(&(*buffer)[0]));

  return 0;
}

int encrypted_file::write_chunk(const char *buffer, size_t size, off_t offset) {
  std::vector<char> temp(size);

  crypto::aes_ctr_256::decrypt_with_byte_offset(
      _data_key, offset, reinterpret_cast<const uint8_t *>(buffer), size,
      reinterpret_cast<uint8_t *>(&temp[0]));

  return file::write_chunk(&temp[0], size, offset);
}
} // namespace fs
} // namespace s3
