#include <boost/lexical_cast.hpp>
#include <pugixml/pugixml.hpp>

#include "file_transfer.hh"
#include "fs.hh"
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

  int download_part(const request::ptr &req, const string &url, int fd, const transfer_part *part)
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

    return 0;
  }

  int upload_part(const request::ptr &req, const string &url, int fd, const string &upload_id, transfer_part *part)
  {
    long rc;

    part->etag = util::compute_md5(fd, MOT_HEX, part->size, part->offset);

    req->init(HTTP_PUT);

    // part numbers are 1-based
    req->set_url(url + "?partNumber=" + lexical_cast<string>(part->id + 1) + "&uploadId=" + upload_id);
    req->set_input_fd(fd, part->size, part->offset);

    req->run();
    rc = req->get_response_code();

    if (rc == 500 || rc == 503)
      return -EAGAIN; // temporary failure
    else if (rc != 200)
      return -EIO;

    if (req->get_response_header("ETag") != part->etag) {
      S3_DEBUG("file_transfer::upload_part", "md5 mismatch. expected %s, got %s.\n", part->etag.c_str(), req->get_response_header("ETag").c_str());
      return -EAGAIN; // assume it's a temporary failure
    }

    return 0;
  }
}

file_transfer::file_transfer(const thread_pool::ptr &tp_fg, const thread_pool::ptr &tp_bg)
  : _tp_fg(tp_fg),
    _tp_bg(tp_bg)
{
}

int file_transfer::__download(const request::ptr &req, const object::ptr &obj, int fd)
{
  int r;
  size_t size = obj->get_size();
  const string &url = obj->get_url(), &expected_md5 = obj->get_md5();

  if (size > g_download_chunk_size)
    r = download_multi(url, size, fd);
  else
    r = download_single(req, url, fd);

  if (r)
    return r;

  fsync(fd);

  // we won't have a valid MD5 digest if the file was a multipart upload
  if (!expected_md5.empty()) {
    string computed_md5 = util::compute_md5(fd, MOT_HEX);

    if (computed_md5 != expected_md5) {
      S3_DEBUG("file_transfer::__download", "md5 mismatch. expected %s, got %s.\n", expected_md5.c_str(), computed_md5.c_str());

      return -EIO;
    }
  }
  
  return 0;
}

int file_transfer::download_single(const request::ptr &req, const string &url, int fd)
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

  return 0;
}

int file_transfer::download_multi(const string &url, size_t size, int fd)
{
  vector<transfer_part> parts((size + g_download_chunk_size - 1) / g_download_chunk_size);
  list<transfer_part *> parts_in_progress;

  for (size_t i = 0; i < parts.size(); i++) {
    transfer_part *part = &parts[i];

    part->id = i;
    part->offset = i * g_download_chunk_size;
    part->size = (i != parts.size() - 1) ? g_download_chunk_size : (size - g_download_chunk_size * i);

    part->handle = _tp_bg->post(bind(&download_part, _1, url, fd, part));
    parts_in_progress.push_back(part);
  }

  while (!parts_in_progress.empty()) {
    transfer_part *part = parts_in_progress.front();
    int result;

    parts_in_progress.pop_front();
    result = _tp_bg->wait(part->handle);

    if (result == -EAGAIN || result == -ETIMEDOUT) {
      S3_DEBUG("file_transfer::download_multi", "part %i returned status %i for [%s].\n", part->id, result, url.c_str());
      part->retry_count++;

      if (part->retry_count > g_max_retries)
        return -EIO;

      part->handle = _tp_bg->post(bind(&download_part, _1, url, fd, part));
      parts_in_progress.push_back(part);

    } else if (result != 0) {
      return result;
    }
  }

  return 0;
}

int file_transfer::__upload(const request::ptr &req, const object::ptr &obj, int fd)
{
  size_t size;

  if (fsync(fd) == -1) {
    S3_DEBUG("file_transfer::__upload", "fsync failed with error %i.\n", errno);
    return -errno;
  }

  size = obj->get_size();

  if (size > g_upload_chunk_size)
    return upload_multi(req, obj, size, fd);
  else
    return upload_single(req, obj, size, fd);
}

int file_transfer::upload_single(const request::ptr &req, const object::ptr &obj, size_t size, int fd)
{
  string returned_md5, etag;
  bool valid_md5;

  req->init(HTTP_PUT);
  req->set_url(obj->get_url());
  req->set_meta_headers(obj);
  req->set_header("Content-MD5", util::compute_md5(fd));
  req->set_input_fd(fd, size);

  req->run();

  if (req->get_response_code() != 200) {
    S3_DEBUG("file_transfer::upload_single", "failed to upload for [%s].\n", obj->get_url().c_str());
    return -EIO;
  }

  etag = req->get_response_header("ETag");
  valid_md5 = util::is_valid_md5(etag);
  returned_md5 = valid_md5 ? etag : (util::compute_md5(fd, MOT_HEX));

  obj->set_md5(returned_md5, etag);

  // we don't need to commit the metadata if we got a valid etag back (since it'll be consistent)
  return valid_md5 ? 0 : fs::commit_metadata(req, obj);
}

int file_transfer::upload_multi(const request::ptr &req, const object::ptr &obj, size_t size, int fd)
{
  const std::string &url = obj->get_url();
  string upload_id, complete_upload;
  bool success = true;
  vector<transfer_part> parts((size + g_upload_chunk_size - 1) / g_upload_chunk_size);
  list<transfer_part *> parts_in_progress;
  xml_document doc;
  xml_parse_result res;
  string etag, computed_md5;

  req->init(HTTP_POST);
  req->set_url(url + "?uploads");
  req->set_meta_headers(obj);

  req->run();

  if (req->get_response_code() != 200)
    return -EIO;

  res = doc.load_buffer(req->get_response_data().data(), req->get_response_data().size());

  if (res.status != status_ok) {
    S3_DEBUG("file_transfer::upload_multi", "failed to parse response: %s\n", res.description());
    return -EIO;
  }

  upload_id = doc.document_element().child_value("UploadId");

  if (upload_id.empty())
    return -EIO;

  for (size_t i = 0; i < parts.size(); i++) {
    transfer_part *part = &parts[i];

    part->id = i;
    part->offset = i * g_upload_chunk_size;
    part->size = (i != parts.size() - 1) ? g_upload_chunk_size : (size - g_upload_chunk_size * i);

    part->handle = _tp_bg->post(bind(&upload_part, _1, url, fd, upload_id, part));
    parts_in_progress.push_back(part);
  }

  while (!parts_in_progress.empty()) {
    transfer_part *part = parts_in_progress.front();
    int result;

    parts_in_progress.pop_front();
    result = _tp_bg->wait(part->handle);

    if (result != 0)
      S3_DEBUG("file_transfer::upload_multi", "part %i returned status %i for [%s].\n", part->id, result, url.c_str());

    // the default action is to not retry the failed part, and leave it with success = false

    if (result == 0) {
      part->success = true;

    } else if (result == -EAGAIN || result == -ETIMEDOUT) {
      part->retry_count++;

      if (part->retry_count <= g_max_retries) {
        part->handle = _tp_bg->post(bind(&upload_part, _1, url, fd, upload_id, part));
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

    // part numbers are 1-based
    complete_upload += "<Part><PartNumber>" + lexical_cast<string>(i + 1) + "</PartNumber><ETag>" + parts[i].etag + "</ETag></Part>";
  }

  complete_upload += "</CompleteMultipartUpload>";

  if (!success) {
    S3_DEBUG("file_transfer::upload_multi", "one or more parts failed to upload for [%s].\n", url.c_str());

    req->init(HTTP_DELETE);
    req->set_url(url + "?uploadId=" + upload_id);

    req->run();

    return -EIO;
  }

  req->init(HTTP_POST);
  req->set_url(url + "?uploadId=" + upload_id);
  req->set_input_data(complete_upload);
  req->set_header("Content-Type", "");

  req->run();

  if (req->get_response_code() != 200) {
    S3_DEBUG("file_transfer::upload_multi", "failed to complete multipart upload for [%s] with error %li.\n", url.c_str(), req->get_response_code());
    return -EIO;
  }

  res = doc.load_buffer(req->get_response_data().data(), req->get_response_data().size());

  if (res.status != status_ok) {
    S3_DEBUG("file_transfer::upload_multi", "failed to parse response: %s\n", res.description());
    return -EIO;
  }

  etag = doc.document_element().child_value("ETag");

  if (etag.empty()) {
    S3_DEBUG("file_transfer::upload_multi", "no etag on multipart upload of [%s]. response: %s\n", url.c_str(), req->get_response_data().c_str());
    return -EIO;
  }

  computed_md5 = util::compute_md5(fd, MOT_HEX);

  // set the MD5 digest manually because the etag we get back is not itself a valid digest
  obj->set_md5(computed_md5, etag);

  return fs::commit_metadata(req, obj);
}
