#include "base/logger.h"
#include "base/request.h"
#include "crypto/aes_cbc_256.h"
#include "crypto/cipher.h"
#include "crypto/hex.h"
#include "crypto/symmetric_key.h"
#include "objects/encrypted_file.h"
#include "services/service.h"

using std::runtime_error;
using std::string;
using std::vector;

using s3::base::request;
using s3::crypto::aes_cbc_256;
using s3::crypto::buffer;
using s3::crypto::cipher;
using s3::crypto::hex;
using s3::crypto::symmetric_key;
using s3::objects::encrypted_file;
using s3::objects::object;
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

  object::type_checker::type_checker s_checker_reg(checker, 100);

  // TODO: move this (and, obviously, make it a config option)
  class keys
  {
  public:
    inline static buffer::ptr get_volume_key()
    {
      return buffer::from_string("9d85a5ee66cf7ba8b313a926be06ca6a5e62b87be88c2e7f45d058d7479fd42e");
    }
  };
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

  _meta_key = symmetric_key::create(
    keys::get_volume_key(), 
    buffer::from_string(req->get_response_header(meta_prefix + "s3fuse-e-iv")));

  meta = cipher::decrypt<aes_cbc_256, hex>(
    _meta_key, 
    req->get_response_header(meta_prefix + "s3fuse-e-meta"));

  if (meta.substr(0, META_VERIFIER.size()) != META_VERIFIER) {
    S3_LOG(
      LOG_WARNING, 
      "encrypted_file::init", 
      "meta decryption failed for [%s]. this probably means the volume key is invalid.\n",
      get_path().c_str());

    throw runtime_error("failed to decrypt file metadata");
  }

  meta = meta.substr(META_VERIFIER.size());
  pos = meta.find('#');

  if (pos == string::npos)
    throw runtime_error("malformed encrypted file metadata");

  _data_key = symmetric_key::from_string(meta.substr(0, pos));
  _expected_root_hash = meta.substr(pos + 1);
}

int encrypted_file::prepare_download()
{
  // create cipher

  return file::prepare_download();
}

int encrypted_file::finalize_download()
{
  // destroy cipher

  return file::finalize_download();
}

int encrypted_file::prepare_upload()
{
  // create cipher

  return file::prepare_upload();
}

int encrypted_file::finalize_upload(const string &returned_etag)
{
  // destroy cipher

  return file::finalize_upload(returned_etag);
}

int encrypted_file::read_chunk(size_t size, off_t offset, vector<char> *buffer)
{
  vector<char> temp;
  int r;

  r = file::read_chunk(size, offset, &temp);

  if (r)
    return r;

  // encrypt temp to buffer

  return 0;
}

int encrypted_file::write_chunk(const char *buffer, size_t size, off_t offset)
{
  vector<char> temp(size);

  // decrypt buffer to temp

  return file::write_chunk(&temp[0], size, offset);
}
