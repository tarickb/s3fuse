#include <boost/lexical_cast.hpp>
#include <pugixml/pugixml.hpp>

#include "file_transfer.hh"
#include "logging.hh"
#include "object.hh"
#include "request.hh"
#include "thread_pool.hh"
#include "util.hh"

using namespace boost;
using namespace pugi;
using namespace std;

using namespace s3;

namespace
{
  const xpath_query UPLOADID_QUERY("/InitiateMultipartUploadResult/UploadId");

  size_t g_download_chunk_size = 128 * 1024; // 128 KiB
  size_t g_upload_chunk_size = 5 * 1024 * 1024; // 5 MiB
  int g_max_retries = 3;

  struct transfer_part
  {
    int id;
    off_t offset;
    size_t size;
    int retry_count;
    bool success;
    string etag;
    async_handle handle;

    inline transfer_part() : id(0), offset(0), size(0), retry_count(0), success(false) { }
  };

  int download_part(const request::ptr &req, const string &url, int fd, const transfer_part *part, string *etag)
  {
    long rc;

    req->init(HTTP_GET);
    req->set_url(url);
    req->set_output_fd(fd, part->offset);
    req->set_header("Range", 
      string("bytes=") + 
      lexical_cast<string>(part->offset) + 
      string("-") + 
      lexical_cast<string>(part->offset + part->size));

    req->run();
    rc = req->get_response_code();

    if (rc == 500 || rc == 503)
      return -EAGAIN; // temporary failure
    else if (rc != 206)
      return -EIO;

    if (etag)
      *etag = req->get_response_header("ETag");

    return 0;
  }

  int upload_part(const request::ptr &req, const string &url, int fd, const string &upload_id, transfer_part *part)
  {
    long rc;
    string expected_md5 = "\"" + util::compute_md5(fd, MOT_HEX, part->offset, part->size) + "\"";

    req->init(HTTP_PUT);
    req->set_url(url + "?partNumber=" + lexical_cast<string>(part->id) + "&uploadId=" + upload_id);
    req->set_input_fd(fd, part->size, part->offset);

    req->run();
    rc = req->get_response_code();

    if (rc == 500 || rc == 503)
      return -EAGAIN; // temporary failure
    else if (rc != 200)
      return -EIO;

    if (req->get_response_header("ETag") != expected_md5) {
      S3_DEBUG("file_transfer::upload_part", "md5 mismatch. expected %s, got %s.\n", expected_md5.c_str(), req->get_response_header("ETag").c_str());
      return -EAGAIN; // assume it's a temporary failure
    }

    return 0;
  }
}

file_transfer::file_transfer(const thread_pool::ptr &pool)
  : _pool(pool)
{ }

int file_transfer::download(const request::ptr &req, const string &url, size_t size, int fd)
{
  int r;
  string expected_md5, computed_md5;

  if (size > g_download_chunk_size)
    r = download_multi(url, size, fd, &expected_md5);
  else
    r = download_single(req, url, fd, &expected_md5);

  if (r)
    return r;

  fsync(fd);
  computed_md5 = "\"" + util::compute_md5(fd, MOT_HEX) + "\"";

  if (computed_md5 != expected_md5) {
    S3_DEBUG("file_transfer::download", "md5 mismatch. expected %s, got %s.\n", expected_md5.c_str(), computed_md5.c_str());

    return -EIO;
  }
  
  return 0;
}

int file_transfer::download_single(const request::ptr &req, const string &url, int fd, string *expected_md5)
{
  long rc;

  req->init(HTTP_GET);
  req->set_url(url);
  req->set_output_fd(fd);

  req->run();
  rc = req->get_response_code();

  if (rc == 404)
    return -ENOENT;
  else if (rc != 200)
    return -EIO;

  *expected_md5 = req->get_response_header("ETag");
  return 0;
}

int file_transfer::download_multi(const string &url, size_t size, int fd, string *expected_md5)
{
  vector<transfer_part> parts((size + g_download_chunk_size - 1) / g_download_chunk_size);
  list<transfer_part *> parts_in_progress;

  for (size_t i = 0; i < parts.size(); i++) {
    transfer_part *part = &parts[i];

    part->id = i;
    part->offset = i * g_download_chunk_size;
    part->size = (i != parts.size()) ? g_download_chunk_size : (size - g_download_chunk_size * i);

    S3_DEBUG("file_transfer::download_multi", "downloading %zu bytes at offset %zd for [%s].\n", part->size, part->offset, url.c_str());

    part->handle = _pool->post(bind(&download_part, _1, url, fd, part, (part->id == 0) ? expected_md5 : NULL));
    parts_in_progress.push_back(part);
  }

  while (!parts_in_progress.empty()) {
    transfer_part *part = parts_in_progress.front();
    int result;

    parts_in_progress.pop_front();
    result = _pool->wait(part->handle);

    S3_DEBUG("file_transfer::download_multi", "part %i returned status %i for [%s].\n", part->id, result, url.c_str());

    if (result == -EAGAIN || result == -ETIMEDOUT) {
      part->retry_count++;

      if (part->retry_count > g_max_retries)
        return -EIO;

      part->handle = _pool->post(bind(&download_part, _1, url, fd, part, (part->id == 0) ? expected_md5 : NULL));
      parts_in_progress.push_back(part);

    } else if (result != 0) {
      return result;
    }
  }

  return 0;
}

int file_transfer::upload(const request::ptr &req, const object::ptr &obj)
{
  size_t size;

  if (!fsync(obj->get_open_file()->get_fd()))
    return -errno;

  size = obj->get_size();
  S3_DEBUG("file_transfer::upload", "writing %zu bytes to [%s].\n", size, obj->get_url().c_str());

  if (size > g_upload_chunk_size)
    return upload_multi(req, obj, size);
  else
    return upload_single(req, obj, size);
}

int file_transfer::upload_single(const request::ptr &req, const object::ptr &obj, size_t size)
{
  req->init(HTTP_PUT);
  req->set_url(obj->get_url());
  req->set_meta_headers(obj);
  req->set_header("Content-MD5", util::compute_md5(obj->get_open_file()->get_fd()));
  req->set_input_fd(obj->get_open_file()->get_fd(), 0, size);

  req->run();

  return (req->get_response_code() == 200) ? 0 : -EIO;
}

int file_transfer::upload_multi(const request::ptr &req, const object::ptr &obj, size_t size)
{
  const std::string &url = obj->get_url();
  int fd = obj->get_open_file()->get_fd();
  string upload_id, complete_upload;
  bool success = true;
  vector<transfer_part> parts((size + g_upload_chunk_size - 1) / g_upload_chunk_size);
  list<transfer_part *> parts_in_progress;
  xml_document doc;

  req->init(HTTP_POST);
  req->set_url(url + "?uploads");
  req->set_meta_headers(obj);

  req->run();

  if (req->get_response_code() != 200)
    return -EIO;

  // TODO: check parse result
  doc.load_buffer(req->get_response_data().data(), req->get_response_data().size());
  upload_id = doc.document_element().child_value("UploadId");

  S3_DEBUG("file_transfer::upload_multi", "upload id %s for file %s.\n", upload_id.c_str(), url.c_str());

  if (upload_id.empty())
    return -EIO;

  for (size_t i = 0; i < parts.size(); i++) {
    transfer_part *part = &parts[i];

    part->id = i;
    part->offset = i * g_upload_chunk_size;
    part->size = (i != parts.size()) ? g_upload_chunk_size : (size - g_upload_chunk_size * i);

    S3_DEBUG("file_transfer::upload_multi", "uploading %zu bytes at offset %zd for [%s].\n", part->size, part->offset, url.c_str());

    part->handle = _pool->post(bind(&upload_part, _1, url, fd, upload_id, part));
    parts_in_progress.push_back(part);
  }

  while (!parts_in_progress.empty()) {
    transfer_part *part = parts_in_progress.front();
    int result;

    parts_in_progress.pop_front();
    result = _pool->wait(part->handle);

    S3_DEBUG("file_transfer::upload_multi", "part %i returned status %i for [%s].\n", part->id, result, url.c_str());

    // the default action is to not retry the failed part, and leave it with success = false

    if (result == 0) {
      part->success = true;

    } else if (result == -EAGAIN || result == -ETIMEDOUT) {
      part->retry_count++;

      if (part->retry_count <= g_max_retries) {
        part->handle = _pool->post(bind(&upload_part, _1, url, fd, upload_id, part));
        parts_in_progress.push_back(part);
      }
    }
  }

  complete_upload = "<CompleteMultipartUpload>";

  for (size_t i = 0; i < parts.size(); i++) {
    if (!parts[i].success) {
      success = false;
      break;
    }

    complete_upload += "<Part><PartNumber>" + lexical_cast<string>(i) + "</PartNumber><ETag>" + parts[i].etag + "</ETag></Part>";
  }

  complete_upload += "</CompleteMultipartUpload>";

  if (!success) {
    S3_DEBUG("file_transfer::upload_multi", "one or more parts failed to upload for [%s].\n", url.c_str());

    req->init(HTTP_DELETE);
    req->set_url(url + "?uploadId=" + upload_id);

    req->run();

    return -EIO;
  }

  req->init(HTTP_PUT);
  req->set_url(url + "?uploadId=" + upload_id);
  req->set_input_data(complete_upload);

  req->run();

  if (req->get_response_code() != 200)
    return -EIO;

  doc.load_buffer(req->get_response_data().data(), req->get_response_data().size());

  if (strlen(doc.document_element().child_value("ETag")) == 0)
    return -EIO;

  return 0;
}
