#include <boost/lexical_cast.hpp>

#include "config.h"
#include "file.h"
#include "logger.h"
#include "object_cache.h"
#include "request.h"
#include "service.h"
#include "thread_pool.h"
#include "util.h"
#include "xattr_reference.h"
#include "xml.h"

using namespace boost;
using namespace std;

using namespace s3;

#define TEMP_NAME_TEMPLATE "/tmp/s3fuse.local-XXXXXX"

namespace
{
  const char *     ETAG_XPATH = "/s3:CompleteMultipartUploadResult/s3:ETag";
  const char *UPLOAD_ID_XPATH = "/s3:InitiateMultipartUploadResult/s3:UploadId";

  object * checker(const string &path, const request::ptr &req)
  {
    return new file(path);
  }

  object::type_checker::type_checker s_checker_reg(checker, 1000);
}

void file::open_locked_object(const object::ptr &obj, file_open_mode mode, uint64_t *handle, int *status)
{
  if (!obj) {
    *status = -ENOENT;
    return;
  }

  if (obj->get_type() != S_IFREG) {
    *status = -EINVAL;
    return;
  }

  *status = static_cast<file *>(obj.get())->open(mode, handle);
}

int file::open(const string &path, file_open_mode mode, uint64_t *handle)
{
  int r = -EINVAL;

  object_cache::lock_object(path, bind(&file::open_locked_object, _1, mode, handle, &r));

  return r;
}

file::file(const string &path)
  : object(path),
    _fd(-1),
    _status(0),
    _async_error(0),
    _ref_count(0)
{
  set_object_type(S_IFREG);
}

file::~file()
{
}

bool file::is_expired()
{
  mutex::scoped_lock lock(_fs_mutex);

  return _ref_count == 0 && object::is_expired();
}

void file::init(const request::ptr &req)
{
  const string &meta_prefix = service::get_header_meta_prefix();

  object::init(req);

  _md5 = req->get_response_header(meta_prefix + "s3fuse-md5");
  _md5_etag = req->get_response_header(meta_prefix + "s3fuse-md5-etag");

  get_metadata()->insert(xattr_reference::from_string("__md5__", &_md5, &_md5_mutex));

  // this workaround is for multipart uploads, which don't get a valid md5 etag
  if (_md5_etag != get_etag() || !util::is_valid_md5(_md5))
    _md5 = util::is_valid_md5(get_etag()) ? get_etag() : "";

  _md5_etag = get_etag();
}

void file::set_request_headers(const request::ptr &req)
{
  const string &meta_prefix = service::get_header_meta_prefix();

  object::set_request_headers(req);

  req->set_header(meta_prefix + "s3fuse-md5", _md5);
  req->set_header(meta_prefix + "s3fuse-md5-etag", _md5_etag);
}

void file::on_download_complete(int ret)
{
  mutex::scoped_lock lock(_fs_mutex);

  if (_status != FS_DOWNLOADING) {
    S3_LOG(LOG_ERR, "file::download_complete", "inconsistent state for [%s]. don't know what to do.\n", get_path().c_str());
    return;
  }

  _async_error = ret;
  _status = 0;
  _condition.notify_all();
}

int file::open(file_open_mode mode, uint64_t *handle)
{
  mutex::scoped_lock lock(_fs_mutex);

  if (_ref_count == 0) {
    char temp_name[] = TEMP_NAME_TEMPLATE;

    _fd = mkstemp(temp_name);
    unlink(temp_name);

    S3_LOG(LOG_DEBUG, "file::open", "opening [%s] in [%s].\n", get_path().c_str(), temp_name);

    if (_fd == -1)
      return -errno;

    if (!(mode & OPEN_TRUNCATE_TO_ZERO)) {
      if (ftruncate(_fd, get_stat()->st_size) != 0)
        return -errno;

      _status = FS_DOWNLOADING;

      thread_pool::post(
        PR_0,
        bind(&file::download, shared_from_this(), _1),
        bind(&file::on_download_complete, shared_from_this(), _1));
    }
  }

  *handle = reinterpret_cast<uint64_t>(this);
  _ref_count++;

  return 0;
}

int file::release()
{
  mutex::scoped_lock lock(_fs_mutex);

  if (_ref_count == 0) {
    S3_LOG(LOG_WARNING, "file::release", "attempt to release file [%s] with zero ref-count\n", get_path().c_str());
    return -EINVAL;
  }

  _ref_count--;

  if (_ref_count == 0) {
    if (_status != 0) {
      S3_LOG(LOG_ERR, "file::release", "released file [%s] with non-quiescent status [%i].\n", get_path().c_str(), _status);
      return -EBUSY;
    }

    close(_fd);
    expire();
  }

  return 0;
}

int file::flush()
{
  mutex::scoped_lock lock(_fs_mutex);

  while (_status & (FS_DOWNLOADING | FS_UPLOADING | FS_WRITING))
    _condition.wait(lock);

  if (!(_status & FS_DIRTY)) {
    S3_LOG(LOG_DEBUG, "file::flush", "skipping flush for non-dirty file [%s].\n", get_path().c_str());
    return 0;
  }

  _status |= FS_UPLOADING;

  lock.unlock();
  _async_error = thread_pool::call(PR_0, bind(&file::upload, shared_from_this(), _1));
  lock.lock();

  _status = 0;
  _condition.notify_all();

  return _async_error;
}

int file::write(const char *buffer, size_t size, off_t offset)
{
  mutex::scoped_lock lock(_fs_mutex);
  int r;

  while (_status & (FS_DOWNLOADING | FS_UPLOADING))
    _condition.wait(lock);

  if (_async_error)
    return _async_error;

  _status |= FS_DIRTY | FS_WRITING;

  lock.unlock();
  r = pwrite(_fd, buffer, size, offset);
  lock.lock();

  _status &= ~FS_WRITING;
  _condition.notify_all();

  return r;
}

int file::read(char *buffer, size_t size, off_t offset)
{
  mutex::scoped_lock lock(_fs_mutex);

  while (_status & FS_DOWNLOADING)
    _condition.wait(lock);

  if (_async_error)
    return _async_error;

  lock.unlock();

  return pread(_fd, buffer, size, offset);
}

int file::truncate(off_t length)
{
  mutex::scoped_lock lock(_fs_mutex);
  int r;

  while (_status & (FS_DOWNLOADING | FS_UPLOADING))
    _condition.wait(lock);

  if (_async_error)
    return _async_error;

  _status |= FS_DIRTY | FS_WRITING;

  lock.unlock();
  r = ftruncate(_fd, length);
  lock.lock();

  _status &= ~FS_WRITING;
  _condition.notify_all();

  return r;
}

int file::write_chunk(const char *buffer, size_t size, off_t offset)
{
  ssize_t r;
  
  r = pwrite(_fd, buffer, size, offset);

  return (r == static_cast<ssize_t>(size)) ? 0 : -errno;
}

int file::read_chunk(size_t size, off_t offset, vector<char> *buffer)
{
  ssize_t r;

  buffer->resize(size);
  r = pread(_fd, &(*buffer)[0], size, offset);

  return (r == static_cast<ssize_t>(size)) ? 0 : -errno;

  // TODO: in encrypted_file (with "size" a multiple of the cipher block size, this should:
  //   - read chunk
  //   - compute md5
  //   - store md5 in vector at index (offset / size)
  //   - encrypt chunk
  //
  // to compute final md5, combine md5s in vector in sequential order -- this will be the plaintext md5
}

size_t file::get_transfer_size()
{
  struct stat s;

  if (fstat(_fd, &s) == -1)
    return 0;

  return s.st_size;
}

void file::copy_stat(struct stat *s)
{
  object::copy_stat(s);

  if (_fd != -1) {
    struct stat real_s;

    if (fstat(_fd, &real_s) != -1)
      s->st_size = real_s.st_size;
  }
}

int file::check_download_consistency()
{
  mutex::scoped_lock lock(_md5_mutex);
  string expected_md5;
 
  expected_md5 = _md5;
  lock.unlock();

  // we won't have a valid MD5 digest if the file was a multipart upload
  if (!expected_md5.empty()) {
    string computed_md5 = util::compute_md5(_fd, E_HEX_WITH_QUOTES);

    if (computed_md5 != expected_md5) {
      S3_LOG(LOG_WARNING, "file::check_download_consistency", "md5 mismatch. expected %s, got %s.\n", expected_md5.c_str(), computed_md5.c_str());

      return -EIO;
    }
  }

  return 0;
}

int file::download(const request::ptr &req)
{
  int r = 0;

  if (service::is_multipart_download_supported() && get_transfer_size() > config::get_download_chunk_size())
    r = download_multi();
  else
    r = thread_pool::call(PR_REQ_1, bind(&file::download_single, shared_from_this(), _1));

  return r ? r : check_download_consistency();
}

int file::download_single(const request::ptr &req)
{
  long rc = 0;

  req->init(HTTP_GET);
  req->set_url(get_url());

  req->run(config::get_transfer_timeout_in_s());
  rc = req->get_response_code();

  if (rc == HTTP_SC_NOT_FOUND)
    return -ENOENT;
  else if (rc != HTTP_SC_OK)
    return -EIO;

  return write_chunk(&req->get_output_buffer()[0], req->get_output_buffer().size(), 0);
}

int file::download_multi()
{
  size_t size = get_transfer_size();
  vector<transfer_part> parts((size + config::get_download_chunk_size() - 1) / config::get_download_chunk_size());
  list<transfer_part *> parts_in_progress;

  // TODO: have at most 8 (?) transfers in progress

  for (size_t i = 0; i < parts.size(); i++) {
    transfer_part *part = &parts[i];

    part->id = i;
    part->offset = i * config::get_download_chunk_size();
    part->size = (i != parts.size() - 1) ? config::get_download_chunk_size() : (size - config::get_download_chunk_size() * i);

    part->handle = thread_pool::post(
      PR_REQ_1, 
      bind(&file::download_part, shared_from_this(), _1, part));

    parts_in_progress.push_back(part);
  }

  while (!parts_in_progress.empty()) {
    transfer_part *part = parts_in_progress.front();
    int result;

    parts_in_progress.pop_front();
    result = part->handle->wait();

    if (result == -EAGAIN || result == -ETIMEDOUT) {
      S3_LOG(LOG_DEBUG, "file::download_multi", "part %i returned status %i for [%s].\n", part->id, result, get_url().c_str());
      part->retry_count++;

      if (part->retry_count > config::get_max_transfer_retries())
        return -EIO;

      part->handle = thread_pool::post(
        PR_REQ_1, 
        bind(&file::download_part, shared_from_this(), _1, part));

      parts_in_progress.push_back(part);

    } else if (result != 0) {
      return result;
    }
  }

  return 0;
}

int file::download_part(const request::ptr &req, const transfer_part *part)
{
  long rc = 0;

  req->init(HTTP_GET);
  req->set_url(get_url());
  req->set_header("Range", 
    string("bytes=") + 
    lexical_cast<string>(part->offset) + 
    string("-") + 
    lexical_cast<string>(part->offset + part->size));

  req->run(config::get_transfer_timeout_in_s());
  rc = req->get_response_code();

  if (rc == HTTP_SC_INTERNAL_SERVER_ERROR || rc == HTTP_SC_SERVICE_UNAVAILABLE)
    return -EAGAIN; // temporary failure
  else if (rc != HTTP_SC_PARTIAL_CONTENT)
    return -EIO;

  return write_chunk(&req->get_output_buffer()[0], req->get_output_buffer().size(), part->offset);
}

int file::upload(const request::ptr &req)
{
  if (service::is_multipart_upload_supported() && get_transfer_size() > config::get_upload_chunk_size())
    return thread_pool::call(PR_REQ_0, bind(&file::upload_multi, shared_from_this(), _1));
  else
    return thread_pool::call(PR_REQ_0, bind(&file::upload_single, shared_from_this(), _1));
}

int file::upload_single(const request::ptr &req)
{
  int r = 0;
  vector<char> buffer;
  vector<uint8_t> md5;
  string expected_md5_b64, expected_md5_hex, etag;
  bool valid_md5;

  r = read_chunk(get_transfer_size(), 0, &buffer);

  if (r)
    return r;

  util::compute_md5(buffer, &md5);

  expected_md5_b64 = util::encode(md5, E_BASE64);
  expected_md5_hex = util::encode(md5, E_HEX_WITH_QUOTES);

  req->init(HTTP_PUT);
  req->set_url(get_url());

  set_request_headers(req);

  req->set_header("Content-MD5", expected_md5_b64);
  req->set_input_buffer(&buffer[0], buffer.size());

  req->run(config::get_transfer_timeout_in_s());

  if (req->get_response_code() != HTTP_SC_OK) {
    S3_LOG(LOG_WARNING, "file::upload_single", "failed to upload for [%s].\n", get_url().c_str());
    return -EIO;
  }

  etag = req->get_response_header("ETag");
  valid_md5 = util::is_valid_md5(etag);

  if (valid_md5 && etag != expected_md5_hex) {
    S3_LOG(LOG_WARNING, "file::upload_single", "etag [%s] does not match md5 [%s].\n", etag.c_str(), expected_md5_hex.c_str());
    return -EIO;
  }
  
  set_etag(etag);
  set_md5(expected_md5_hex, etag);

  // we don't need to commit the metadata if we got a valid etag back (since it'll be consistent)
  return valid_md5 ? 0 : commit(req);
}

int file::upload_multi(const request::ptr &req)
{
  string upload_id, complete_upload;
  bool success = true;
  size_t size = get_transfer_size();
  vector<transfer_part> parts((size + config::get_upload_chunk_size() - 1) / config::get_upload_chunk_size());
  list<transfer_part *> parts_in_progress;
  xml::document doc;
  string etag, computed_md5;
  int r;

  req->init(HTTP_POST);
  req->set_url(get_url() + "?uploads");

  set_request_headers(req);

  req->run();

  if (req->get_response_code() != HTTP_SC_OK)
    return -EIO;

  doc = xml::parse(req->get_output_string());

  if (!doc) {
    S3_LOG(LOG_WARNING, "file::upload_multi", "failed to parse response.\n");
    return -EIO;
  }

  if ((r = xml::find(doc, UPLOAD_ID_XPATH, &upload_id)))
    return r;

  if (upload_id.empty())
    return -EIO;

  for (size_t i = 0; i < parts.size(); i++) {
    transfer_part *part = &parts[i];

    part->id = i;
    part->offset = i * config::get_upload_chunk_size();
    part->size = (i != parts.size() - 1) ? config::get_upload_chunk_size() : (size - config::get_upload_chunk_size() * i);

    part->handle = thread_pool::post(
      PR_REQ_1, 
      bind(&file::upload_part, shared_from_this(), _1, upload_id, part));

    parts_in_progress.push_back(part);
  }

  while (!parts_in_progress.empty()) {
    transfer_part *part = parts_in_progress.front();
    int result;

    parts_in_progress.pop_front();
    result = part->handle->wait();

    if (result != 0)
      S3_LOG(LOG_DEBUG, "file::upload_multi", "part %i returned status %i for [%s].\n", part->id, result, get_url().c_str());

    // the default action is to not retry the failed part, and leave it with success = false

    if (result == 0) {
      part->success = true;

    } else if (result == -EAGAIN || result == -ETIMEDOUT) {
      part->retry_count++;

      if (part->retry_count <= config::get_max_transfer_retries()) {
        part->handle = thread_pool::post(
          PR_REQ_1, 
          bind(&file::upload_part, shared_from_this(), _1, upload_id, part));

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
    S3_LOG(LOG_WARNING, "file::upload_multi", "one or more parts failed to upload for [%s].\n", get_url().c_str());

    req->init(HTTP_DELETE);
    req->set_url(get_url() + "?uploadId=" + upload_id);

    req->run();

    return -EIO;
  }

  req->init(HTTP_POST);
  req->set_url(get_url() + "?uploadId=" + upload_id);
  req->set_input_buffer(complete_upload);
  req->set_header("Content-Type", "");

  // use the transfer timeout because completing a multi-part upload can take a long time
  // see http://docs.amazonwebservices.com/AmazonS3/latest/API/index.html?mpUploadComplete.html
  req->run(config::get_transfer_timeout_in_s());

  if (req->get_response_code() != HTTP_SC_OK) {
    S3_LOG(LOG_WARNING, "file::upload_multi", "failed to complete multipart upload for [%s] with error %li.\n", get_url().c_str(), req->get_response_code());
    return -EIO;
  }

  doc = xml::parse(req->get_output_string());

  if (!doc) {
    S3_LOG(LOG_WARNING, "file::upload_multi", "failed to parse response.\n");
    return -EIO;
  }

  if ((r = xml::find(doc, ETAG_XPATH, &etag)))
    return r;

  if (etag.empty()) {
    S3_LOG(LOG_WARNING, "file::upload_multi", "no etag on multipart upload of [%s]. response: %s\n", get_url().c_str(), req->get_output_string().c_str());
    return -EIO;
  }

  // TODO: switch to hash tree to compute message segments in parallel?
  computed_md5 = util::compute_md5(_fd, E_HEX_WITH_QUOTES);

  // set the MD5 digest manually because the etag we get back is not itself a valid digest
  set_etag(etag);
  set_md5(computed_md5, etag);

  return commit(req);
}

int file::upload_part(const request::ptr &req, const string &upload_id, transfer_part *part)
{
  int r = 0;
  long rc = 0;
  vector<char> buffer;

  r = read_chunk(part->size, part->offset, &buffer);

  if (r)
    return r;

  part->etag = util::compute_md5(buffer, E_HEX_WITH_QUOTES);

  req->init(HTTP_PUT);

  // part numbers are 1-based
  req->set_url(get_url() + "?partNumber=" + lexical_cast<string>(part->id + 1) + "&uploadId=" + upload_id);
  req->set_input_buffer(&buffer[0], buffer.size());

  req->run(config::get_transfer_timeout_in_s());
  rc = req->get_response_code();

  if (rc == HTTP_SC_INTERNAL_SERVER_ERROR || rc == HTTP_SC_SERVICE_UNAVAILABLE)
    return -EAGAIN; // temporary failure
  else if (rc != HTTP_SC_OK)
    return -EIO;

  if (req->get_response_header("ETag") != part->etag) {
    S3_LOG(LOG_WARNING, "file::upload_part", "md5 mismatch. expected %s, got %s.\n", part->etag.c_str(), req->get_response_header("ETag").c_str());
    return -EAGAIN; // assume it's a temporary failure
  }

  return 0;
}
