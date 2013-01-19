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

#include <boost/detail/atomic_count.hpp>

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

using boost::detail::atomic_count;
using std::ostream;
using std::runtime_error;
using std::string;
using std::vector;

using s3::base::request;
using s3::base::statistics;
using s3::crypto::aes_cbc_256_with_pkcs;
using s3::crypto::aes_ctr_256;
using s3::crypto::buffer;
using s3::crypto::cipher;
using s3::crypto::hex;
using s3::crypto::symmetric_key;
using s3::fs::encrypted_file;
using s3::fs::encryption;
using s3::fs::metadata;
using s3::fs::object;
using s3::services::service;

namespace
{
  const string CONTENT_TYPE = "binary/encrypted-s3fuse-file_0100"; // version 1.0
  const string META_VERIFIER = "s3fuse_enc_meta ";

  atomic_count s_no_iv_or_meta(0), s_init_errors(0), s_open_without_key(0);

  object * checker(const string &path, const request::ptr &req)
  {
    if (req->get_response_header("Content-Type") != CONTENT_TYPE)
      return NULL;

    return new encrypted_file(path);
  }

  void statistics_writer(ostream *o)
  {
    *o <<
      "encrypted_file:\n"
      "  init without iv or metadata: " << s_no_iv_or_meta << "\n"
      "  init errors: " << s_init_errors << "\n"
      "  open without key: " << s_open_without_key << "\n";
  }

  object::type_checker_list::entry s_checker_reg(checker, 100);
  statistics::writers::entry s_writer(statistics_writer, 0);
}

encrypted_file::encrypted_file(const string &path)
  : file(path)
{
  set_content_type(CONTENT_TYPE);
}

encrypted_file::~encrypted_file()
{
}

void encrypted_file::init(const request::ptr &req)
{
  const string &meta_prefix = service::get_header_meta_prefix();
  string meta;
  size_t pos;

  file::init(req);

  if (!is_intact()) {
    S3_LOG(
      LOG_DEBUG,
      "encrypted_file::init",
      "file [%s] is not intact\n",
      get_path().c_str());

    return;
  }

  _enc_iv = req->get_response_header(meta_prefix + metadata::ENC_IV);
  _enc_meta = req->get_response_header(meta_prefix + metadata::ENC_METADATA);

  if (_enc_iv.empty() || _enc_meta.empty()) {
    ++s_no_iv_or_meta;

    S3_LOG(
      LOG_DEBUG,
      "encrypted_file::init",
      "file [%s] has no IV/metadata\n",
      get_path().c_str());

    return;
  }

  try {
    _meta_key = symmetric_key::create(encryption::get_volume_key(), buffer::from_string(_enc_iv));

    try {
      meta = cipher::decrypt<aes_cbc_256_with_pkcs, hex>(_meta_key, _enc_meta);
    } catch (...) {
      throw runtime_error("failed to decrypt file metadata. this probably means the volume key is invalid.");
    }

    if (meta.substr(0, META_VERIFIER.size()) != META_VERIFIER)
      throw runtime_error("file metadata not valid. this probably means the volume key is invalid.");

    meta = meta.substr(META_VERIFIER.size());
    pos = meta.find('#');

    if (pos == string::npos)
      throw runtime_error("malformed encrypted file metadata");

    _data_key = symmetric_key::from_string(meta.substr(0, pos));

    set_sha256_hash(meta.substr(pos + 1));

  } catch (const std::exception &e) {
    ++s_init_errors;

    S3_LOG(
      LOG_WARNING,
      "encrypted_file::init",
      "caught exception while initializing [%s]: %s\n",
      get_path().c_str(),
      e.what());

    _meta_key.reset();
    _data_key.reset();
  }

  // by not throwing an exception when something goes wrong here, we leave a
  // usable object that can be renamed/moved/etc. but that cannot be opened.
}

void encrypted_file::set_request_headers(const request::ptr &req)
{
  const string &meta_prefix = service::get_header_meta_prefix();

  file::set_request_headers(req);

  // hide the real hash
  req->set_header(meta_prefix + metadata::SHA256, "");

  req->set_header(meta_prefix + metadata::ENC_IV, _enc_iv);
  req->set_header(meta_prefix + metadata::ENC_METADATA, _enc_meta);
}

int encrypted_file::is_downloadable()
{
  if (!_data_key) {
    ++s_open_without_key;

    S3_LOG(
      LOG_DEBUG, 
      "encrypted_file::is_downloadable", 
      "cannot open [%s] without key\n", 
      get_path().c_str());

    return -EACCES;
  }

  return 0;
}

int encrypted_file::prepare_upload()
{
  _meta_key = symmetric_key::generate<aes_cbc_256_with_pkcs>(encryption::get_volume_key());
  _data_key = symmetric_key::generate<aes_ctr_256>();

  // TODO: update _enc_iv and _enc_meta here so that if the upload finishes 
  // but the commit fails, we don't end up with an un-openable object.  maybe 
  // set _enc_meta to a value that indicates we have a zombie or something.
  // maybe even report the file size as zero?

  _enc_iv.clear();
  _enc_meta.clear();

  return file::prepare_upload();
}

int encrypted_file::finalize_upload(const string &returned_etag)
{
  int r;

  r = file::finalize_upload(returned_etag);

  if (r)
    return r;

  _enc_iv = _meta_key->get_iv()->to_string();
  _enc_meta = cipher::encrypt<aes_cbc_256_with_pkcs, hex>(
    _meta_key, 
    META_VERIFIER + _data_key->to_string() + "#" + get_sha256_hash());

  return 0;
}

int encrypted_file::read_chunk(size_t size, off_t offset, vector<char> *buffer)
{
  vector<char> temp;
  int r;

  r = file::read_chunk(size, offset, &temp);

  if (r)
    return r;

  buffer->resize(temp.size());

  aes_ctr_256::encrypt_with_byte_offset(
    _data_key, 
    offset,
    reinterpret_cast<const uint8_t *>(&temp[0]), 
    temp.size(), 
    reinterpret_cast<uint8_t *>(&(*buffer)[0]));

  return 0;
}

int encrypted_file::write_chunk(const char *buffer, size_t size, off_t offset)
{
  vector<char> temp(size);

  aes_ctr_256::decrypt_with_byte_offset(
    _data_key, 
    offset,
    reinterpret_cast<const uint8_t *>(buffer), 
    size, 
    reinterpret_cast<uint8_t *>(&temp[0]));

  return file::write_chunk(&temp[0], size, offset);
}
