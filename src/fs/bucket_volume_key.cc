#include "base/request.h"
#include "crypto/buffer.h"
#include "crypto/cipher.h"
#include "crypto/symmetric_key.h"
#include "fs/bucket_volume_key.h"
#include "fs/object.h"
#include "services/service.h"

using std::runtime_error;
using std::string;

using s3::base::http_method;
using s3::base::request;
using s3::crypto::buffer;
using s3::crypto::cipher;
using s3::crypto::hex;
using s3::crypto::symmetric_key;
using s3::fs::bucket_volume_key;
using s3::fs::object;
using s3::services::service;

namespace
{
  const string VOLUME_KEY_OBJECT = "encryption_vk";
  const string VOLUME_KEY_OBJECT_TEMP = "encryption_vk_temp";

  const string VOLUME_KEY_PREFIX = "s3fuse-00 ";

  request::ptr build_request(http_method method, const string &object = VOLUME_KEY_OBJECT)
  {
    request::ptr req(new request());

    req->init(method);
    req->set_hook(service::get_request_hook());
    req->set_url(object::build_internal_url(object));

    return req;
  }

  string build_bucket_meta(const buffer::ptr &volume_key, const buffer::ptr &password_key)
  {
    string bucket_meta;

    bucket_meta = cipher::encrypt<bucket_volume_key::key_cipher, hex>(
      symmetric_key::create(password_key, buffer::zero(bucket_volume_key::key_cipher::IV_LEN)),
      VOLUME_KEY_PREFIX + volume_key->to_string());

    return bucket_meta;
  }
}

bool bucket_volume_key::is_present()
{
  request::ptr req = build_request(s3::base::HTTP_HEAD);

  req->run();

  if (req->get_response_code() == s3::base::HTTP_SC_OK)
    return true;

  if (req->get_response_code() == s3::base::HTTP_SC_NOT_FOUND)
    return false;

  throw runtime_error("request for volume key object failed.");
}

buffer::ptr bucket_volume_key::read(const buffer::ptr &key)
{
  string bucket_meta;
  request::ptr req = build_request(s3::base::HTTP_GET);

  req->run();

  if (req->get_response_code() == s3::base::HTTP_SC_NOT_FOUND)
    throw runtime_error("bucket does not contain an encryption key. create one with s3fuse_vol_key.");

  if (req->get_response_code() != s3::base::HTTP_SC_OK)
    throw runtime_error("error while fetching bucket encryption key. try re-creating one with s3fuse_vol_key.");

  try {
    bucket_meta = cipher::decrypt<key_cipher, hex>(
      symmetric_key::create(key, buffer::zero(key_cipher::IV_LEN)), 
      req->get_output_string());
  } catch (...) {
    throw runtime_error("incorrect key.");
  }

  if (bucket_meta.substr(0, VOLUME_KEY_PREFIX.size()) != VOLUME_KEY_PREFIX)
    throw runtime_error("incorrect key.");

  return buffer::from_string(bucket_meta.substr(VOLUME_KEY_PREFIX.size()));
}

void bucket_volume_key::generate(const buffer::ptr &key)
{
  buffer::ptr volume_key;
  request::ptr req;

  volume_key = buffer::generate(key_cipher::DEFAULT_KEY_LEN);
  req = build_request(s3::base::HTTP_PUT);

  req->set_input_buffer(build_bucket_meta(volume_key, key));
  req->run();

  if (req->get_response_code() != s3::base::HTTP_SC_OK)
    throw runtime_error("failed to store volume key.");
}

void bucket_volume_key::reencrypt(const buffer::ptr &old_key, const buffer::ptr &new_key)
{
  buffer::ptr volume_key;
  request::ptr req;
  string etag;

  volume_key = read(old_key);

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

void bucket_volume_key::remove()
{
  request req;

  req.init(s3::base::HTTP_DELETE);
  req.set_hook(service::get_request_hook());
  req.set_url(object::build_internal_url(VOLUME_KEY_OBJECT));
  req.run();

  if (req.get_response_code() != s3::base::HTTP_SC_NO_CONTENT)
    throw runtime_error("failed to delete volume key.");
}
