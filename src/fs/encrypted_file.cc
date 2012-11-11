#include "base/logger.h"
#include "base/request.h"
#include "crypto/aes_cbc_256.h"
#include "crypto/aes_ctr_256.h"
#include "crypto/cipher.h"
#include "crypto/hex.h"
#include "crypto/symmetric_key.h"
#include "fs/encrypted_file.h"
#include "fs/encryption.h"
#include "fs/metadata.h"
#include "services/service.h"

using std::runtime_error;
using std::string;
using std::vector;

using s3::base::request;
using s3::crypto::aes_cbc_256;
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

  object * checker(const string &path, const request::ptr &req)
  {
    if (req->get_response_header("Content-Type") != CONTENT_TYPE)
      return NULL;

    return new encrypted_file(path);
  }

  object::type_checker_list::entry s_checker_reg(checker, 100);
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
      meta = cipher::decrypt<aes_cbc_256, hex>(_meta_key, _enc_meta);
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

int encrypted_file::prepare_download()
{
  return file::prepare_download();
}

int encrypted_file::finalize_download()
{
  return file::finalize_download();
}

int encrypted_file::prepare_upload()
{
  _meta_key = symmetric_key::generate<aes_cbc_256>(encryption::get_volume_key());
  _data_key = symmetric_key::generate<aes_ctr_256>();

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
  _enc_meta = cipher::encrypt<aes_cbc_256, hex>(
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

  aes_ctr_256::create_with_byte_offset(_data_key, offset)->encrypt(
    reinterpret_cast<const uint8_t *>(&temp[0]), 
    temp.size(), 
    reinterpret_cast<uint8_t *>(&(*buffer)[0]));

  return 0;
}

int encrypted_file::write_chunk(const char *buffer, size_t size, off_t offset)
{
  vector<char> temp(size);

  // test this here rather than in prepare_download() so that we only fail
  // if we're asked to decrypt non-empty files.

  if (size == 0)
    return 0;

  if (!_data_key)
    return -EACCES;

  aes_ctr_256::create_with_byte_offset(_data_key, offset)->decrypt(
    reinterpret_cast<const uint8_t *>(buffer), 
    size, 
    reinterpret_cast<uint8_t *>(&temp[0]));

  return file::write_chunk(&temp[0], size, offset);
}
