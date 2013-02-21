#include <vector>
#include <boost/lexical_cast.hpp>

#include "base/config.h"
#include "base/logger.h"
#include "base/xml.h"
#include "crypto/hash.h"
#include "crypto/hex_with_quotes.h"
#include "crypto/md5.h"
#include "services/aws_file_transfer.h"
#include "threads/pool.h"

using boost::lexical_cast;
using boost::scoped_ptr;
using std::string;
using std::vector;

using s3::base::char_vector;
using s3::base::char_vector_ptr;
using s3::base::config;
using s3::base::request;
using s3::base::xml;
using s3::crypto::hash;
using s3::crypto::hex_with_quotes;
using s3::crypto::md5;
using s3::services::aws_file_transfer;
using s3::threads::pool;

namespace
{
  const size_t UPLOAD_CHUNK_SIZE = 5 * 1024 * 1024;

  const char *MULTIPART_ETAG_XPATH = "/CompleteMultipartUploadResult/ETag";
  const char *MULTIPART_UPLOAD_ID_XPATH = "/InitiateMultipartUploadResult/UploadId";
}

aws_file_transfer::aws_file_transfer()
{
  _upload_chunk_size = 
    (config::get_upload_chunk_size() == -1)
      ? UPLOAD_CHUNK_SIZE
      : config::get_upload_chunk_size();
}

int aws_file_transfer::upload(const string &url, size_t size, const read_chunk_fn &on_read, string *returned_etag)
{
  if (size > _upload_chunk_size)
    return upload_multi(url, size, on_read, returned_etag);
  else
    return pool::call(s3::threads::PR_REQ_1, bind(&file_transfer::upload_single, _1, url, size, on_read, returned_etag));
}

size_t aws_file_transfer::get_upload_chunk_size()
{
  return _upload_chunk_size;
}

int aws_file_transfer::upload_part(
  const request::ptr &req, 
  const string &url, 
  const string &upload_id, 
  const read_chunk_fn &on_read, 
  upload_range *range, 
  bool is_retry)
{
  int r = 0;
  long rc = 0;
  char_vector_ptr buffer(new char_vector());

  r = on_read(range->size, range->offset, buffer);

  if (r)
    return r;

  range->etag = hash::compute<md5, hex_with_quotes>(*buffer);

  req->init(s3::base::HTTP_PUT);

  // part numbers are 1-based
  req->set_url(url + "?partNumber=" + lexical_cast<string>(range->id + 1) + "&uploadId=" + upload_id);
  req->set_input_buffer(buffer);

  req->run(config::get_transfer_timeout_in_s());
  rc = req->get_response_code();

  if (rc != s3::base::HTTP_SC_OK)
    return -EIO;

  if (req->get_response_header("ETag") != range->etag) {
    S3_LOG(LOG_WARNING, "aws_file_transfer::upload_part", "md5 mismatch. expected %s, got %s.\n", range->etag.c_str(), req->get_response_header("ETag").c_str());
    return -EAGAIN; // assume it's a temporary failure
  }

  return 0;
}

int aws_file_transfer::upload_multi_init(const request::ptr &req, const string &url, string *upload_id)
{
  xml::document doc;
  int r;

  req->init(s3::base::HTTP_POST);
  req->set_url(url + "?uploads");

  req->run();

  if (req->get_response_code() != s3::base::HTTP_SC_OK)
    return -EIO;

  doc = xml::parse(req->get_output_string());

  if (!doc) {
    S3_LOG(LOG_WARNING, "aws_file_transfer::upload_multi_init", "failed to parse response.\n");
    return -EIO;
  }

  if ((r = xml::find(doc, MULTIPART_UPLOAD_ID_XPATH, upload_id)))
    return r;

  if (upload_id->empty())
    return -EIO;

  return r;
}

int aws_file_transfer::upload_multi_cancel(const request::ptr &req, const string &url, const string &upload_id)
{
  S3_LOG(LOG_WARNING, "aws_file_transfer::upload_multi_cancel", "one or more parts failed to upload for [%s].\n", url.c_str());

  req->init(s3::base::HTTP_DELETE);
  req->set_url(url + "?uploadId=" + upload_id);

  req->run();

  return 0;
}

int aws_file_transfer::upload_multi_complete(
  const request::ptr &req, 
  const string &url, 
  const string &upload_id, 
  const string &upload_metadata, 
  string *etag)
{
  xml::document doc;
  int r;

  req->init(s3::base::HTTP_POST);
  req->set_url(url + "?uploadId=" + upload_id);
  req->set_input_buffer(upload_metadata);
  req->set_header("Content-Type", "");

  // use the transfer timeout because completing a multi-part upload can take a long time
  // see http://docs.amazonwebservices.com/AmazonS3/latest/API/index.html?mpUploadComplete.html
  req->run(config::get_transfer_timeout_in_s());

  if (req->get_response_code() != s3::base::HTTP_SC_OK) {
    S3_LOG(LOG_WARNING, "aws_file_transfer::upload_multi_complete", "failed to complete multipart upload for [%s] with error %li.\n", url.c_str(), req->get_response_code());
    return -EIO;
  }

  doc = xml::parse(req->get_output_string());

  if (!doc) {
    S3_LOG(LOG_WARNING, "aws_file_transfer::upload_multi_complete", "failed to parse response.\n");
    return -EIO;
  }

  if ((r = xml::find(doc, MULTIPART_ETAG_XPATH, etag)))
    return r;

  if (etag->empty()) {
    S3_LOG(LOG_WARNING, "aws_file_transfer::upload_multi_complete", "no etag on multipart upload of [%s]. response: %s\n", url.c_str(), req->get_output_string().c_str());
    return -EIO;
  }

  return 0;
}

int aws_file_transfer::upload_multi(const string &url, size_t size, const read_chunk_fn &on_read, string *returned_etag)
{
  string upload_id, complete_upload;
  const size_t num_parts = (size + _upload_chunk_size - 1) / _upload_chunk_size;
  vector<upload_range> parts(num_parts);
  scoped_ptr<multipart_upload> upload;
  int r;

  r = pool::call(s3::threads::PR_REQ_0, bind(&aws_file_transfer::upload_multi_init, this, _1, url, &upload_id));

  if (r)
    return r;

  for (size_t i = 0; i < num_parts; i++) {
    upload_range *part = &parts[i];

    part->id = i;
    part->offset = i * _upload_chunk_size;
    part->size = (i != num_parts - 1) ? _upload_chunk_size : (size - _upload_chunk_size * i);
  }

  upload.reset(new multipart_upload(
    parts,
    bind(&aws_file_transfer::upload_part, this, _1, url, upload_id, on_read, _2, false),
    bind(&aws_file_transfer::upload_part, this, _1, url, upload_id, on_read, _2, true)));

  r = upload->process();

  if (r) {
    pool::call(
      s3::threads::PR_REQ_0, 
      bind(&aws_file_transfer::upload_multi_cancel, this, _1, url, upload_id));

    return r;
  }

  complete_upload = "<CompleteMultipartUpload>";

  for (size_t i = 0; i < parts.size(); i++) {
    // part numbers are 1-based
    complete_upload += "<Part><PartNumber>" + lexical_cast<string>(i + 1) + "</PartNumber><ETag>" + parts[i].etag + "</ETag></Part>";
  }

  complete_upload += "</CompleteMultipartUpload>";

  return pool::call(
    s3::threads::PR_REQ_0, 
    bind(&aws_file_transfer::upload_multi_complete, this, _1, url, upload_id, complete_upload, returned_etag));
}
