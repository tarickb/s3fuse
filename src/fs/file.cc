#include <boost/lexical_cast.hpp>

#include "base/config.h"
#include "base/logger.h"
#include "base/request.h"
#include "base/xml.h"
#include "crypto/base64.h"
#include "crypto/encoder.h"
#include "crypto/hash.h"
#include "crypto/hex.h"
#include "crypto/hex_with_quotes.h"
#include "crypto/md5.h"
#include "fs/cache.h"
#include "fs/metadata.h"
#include "fs/file.h"
#include "services/service.h"
#include "threads/pool.h"

using boost::lexical_cast;
using boost::mutex;
using std::list;
using std::runtime_error;
using std::string;
using std::vector;

using s3::base::config;
using s3::base::request;
using s3::base::xml;
using s3::crypto::base64;
using s3::crypto::encoder;
using s3::crypto::hash;
using s3::crypto::hash_list;
using s3::crypto::hex;
using s3::crypto::hex_with_quotes;
using s3::crypto::md5;
using s3::crypto::sha256;
using s3::fs::file;
using s3::fs::metadata;
using s3::fs::object;
using s3::services::service;
using s3::threads::pool;

#define TEMP_NAME_TEMPLATE "/tmp/s3fuse.local-XXXXXX"

namespace
{
  const size_t MAX_PARTS_IN_PROGRESS = 4;
  const char *ETAG_XPATH = "/s3:CompleteMultipartUploadResult/s3:ETag";
  const char *UPLOAD_ID_XPATH = "/s3:InitiateMultipartUploadResult/s3:UploadId";

  object * checker(const string &path, const request::ptr &req)
  {
    return new file(path);
  }

  object::type_checker_list::entry s_checker_reg(checker, 1000);
}

void file::test_transfer_chunk_sizes()
{
  size_t chunk_size = hash_list<sha256>::CHUNK_SIZE;

  if (config::get_download_chunk_size() % chunk_size) {
    S3_LOG(LOG_ERR, "file::test_transfer_chunk_size", "download chunk size must be a multiple of %zu.\n", chunk_size);
    throw runtime_error("invalid download chunk size");
  }

  if (config::get_upload_chunk_size() % chunk_size) {
    S3_LOG(LOG_ERR, "file::test_transfer_chunk_size", "upload chunk size must be a multiple of %zu.\n", chunk_size);
    throw runtime_error("invalid upload chunk size");
  }
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

  cache::lock_object(path, bind(&file::open_locked_object, _1, mode, handle, &r));

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

bool file::is_removable()
{
  mutex::scoped_lock lock(_fs_mutex);

  return _ref_count == 0 && object::is_removable();
}

void file::init(const request::ptr &req)
{
  const string &meta_prefix = service::get_header_meta_prefix();

  object::init(req);

  if (is_intact()) {
    // we were the last people to modify this object, so everything should be
    // as we left it

    set_sha256_hash(req->get_response_header(meta_prefix + metadata::SHA256));
  }
}

void file::set_sha256_hash(const string &hash)
{
  if (hash.empty())
    return;

  _sha256_hash = hash;
  get_metadata()->replace(xattr::from_string("s3fuse_sha256", hash));
}

void file::set_request_headers(const request::ptr &req)
{
  const string &meta_prefix = service::get_header_meta_prefix();

  object::set_request_headers(req);

  req->set_header(meta_prefix + metadata::SHA256, _sha256_hash);
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

    if (!(mode & fs::OPEN_TRUNCATE_TO_ZERO)) {
      if (ftruncate(_fd, get_stat()->st_size) != 0)
        return -errno;

      _status = FS_DOWNLOADING;

      pool::post(
        threads::PR_0,
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
  _async_error = pool::call(threads::PR_0, bind(&file::upload, shared_from_this(), _1));
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

  if (r != static_cast<ssize_t>(size))
    return -errno;

  if (_hash_list)
    _hash_list->compute_hash(offset, reinterpret_cast<const uint8_t *>(buffer), size);

  return 0;
}

int file::read_chunk(size_t size, off_t offset, vector<char> *buffer)
{
  ssize_t r;

  buffer->resize(size);
  r = pread(_fd, &(*buffer)[0], size, offset);

  if (r != static_cast<ssize_t>(size))
    return -errno;

  if (_hash_list)
    _hash_list->compute_hash(offset, reinterpret_cast<const uint8_t *>(&(*buffer)[0]), size);

  return 0;
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

int file::download(const request::ptr & /* ignored */)
{
  int r = 0;

  r = prepare_download();

  if (r)
    return r;

  if (service::is_multipart_download_supported() && get_transfer_size() > config::get_download_chunk_size())
    r = download_multi();
  else
    r = pool::call(threads::PR_REQ_1, bind(&file::download_single, shared_from_this(), _1));

  return r ? r : finalize_download();
}

int file::prepare_download()
{
  if (!_sha256_hash.empty())
    _hash_list.reset(new hash_list<sha256>(get_transfer_size()));

  return 0;
}

int file::finalize_download()
{
  if (!_sha256_hash.empty()) {
    string computed_hash = _hash_list->get_root_hash<hex>();

    if (computed_hash != _sha256_hash) {
      S3_LOG(
        LOG_WARNING, 
        "file::finalize_download", 
        "sha256 mismatch for %s. expected %s, got %s.\n",
        get_path().c_str(),
        _sha256_hash.c_str(),
        computed_hash.c_str());

      return -EIO;
    }
  } else if (md5::is_valid_quoted_hex_hash(get_etag())) {
    // as a fallback, use the etag as an md5 hash of the file
    string computed_hash = hash::compute<md5, hex_with_quotes>(_fd);

    if (computed_hash != get_etag()) {
      S3_LOG(
        LOG_WARNING,
        "file::finalize_download",
        "md5 mismatch for %s. expected %s, got %s.\n",
        get_path().c_str(),
        computed_hash.c_str(),
        get_etag().c_str());

      return -EIO;
    }
  } else {
    S3_LOG(
      LOG_WARNING,
      "file::finalize_download",
      "no hash check performed for %s\n",
      get_path().c_str());
  }

  return 0;
}

int file::download_single(const request::ptr &req)
{
  long rc = 0;

  req->init(base::HTTP_GET);
  req->set_url(get_url());

  req->run(config::get_transfer_timeout_in_s());
  rc = req->get_response_code();

  if (rc == base::HTTP_SC_NOT_FOUND)
    return -ENOENT;
  else if (rc != base::HTTP_SC_OK)
    return -EIO;

  return write_chunk(&req->get_output_buffer()[0], req->get_output_buffer().size(), 0);
}

int file::download_multi()
{
  size_t size = get_transfer_size();
  size_t num_parts = (size + config::get_download_chunk_size() - 1) / config::get_download_chunk_size();
  size_t last_part = 0;
  vector<transfer_part> parts(num_parts);
  list<transfer_part *> parts_in_progress;
  int r = 0;

  for (size_t i = 0; i < num_parts; i++) {
    transfer_part *part = &parts[i];

    part->id = i;
    part->offset = i * config::get_download_chunk_size();
    part->size = (i != num_parts - 1) ? config::get_download_chunk_size() : (size - config::get_download_chunk_size() * i);
  }

  for (last_part = 0; last_part < std::min(MAX_PARTS_IN_PROGRESS, num_parts); last_part++) {
    transfer_part *part = &parts[last_part];

    part->handle = pool::post(
      threads::PR_REQ_1, 
      bind(&file::download_part, shared_from_this(), _1, part));

    parts_in_progress.push_back(part);
  }

  while (!parts_in_progress.empty()) {
    transfer_part *part = parts_in_progress.front();
    int part_r;

    parts_in_progress.pop_front();
    part_r = part->handle->wait();

    if (part_r == -EAGAIN || part_r == -ETIMEDOUT) {
      S3_LOG(LOG_DEBUG, "file::download_multi", "part %i returned status %i for [%s].\n", part->id, part_r, get_url().c_str());
      part->retry_count++;

      if (part->retry_count > config::get_max_transfer_retries()) {
        part_r = -EIO;
      } else {
        part->handle = pool::post(
          threads::PR_REQ_1, 
          bind(&file::download_part, shared_from_this(), _1, part));

        parts_in_progress.push_back(part);
      }
    }

    if (r == 0) // only save the first non-successful return code
      r = part_r;

    // keep collecting parts until we have nothing left pending
    // if one part fails, keep going but stop posting new parts

    if (r == 0 && last_part < num_parts) {
      part = &parts[last_part++];

      part->handle = pool::post(
        threads::PR_REQ_1, 
        bind(&file::download_part, shared_from_this(), _1, part));

      parts_in_progress.push_back(part);
    }
  }

  return r;
}

int file::download_part(const request::ptr &req, const transfer_part *part)
{
  long rc = 0;

  req->init(base::HTTP_GET);
  req->set_url(get_url());
  req->set_header("Range", 
    string("bytes=") + 
    lexical_cast<string>(part->offset) + 
    string("-") + 
    lexical_cast<string>(part->offset + part->size));

  req->run(config::get_transfer_timeout_in_s());
  rc = req->get_response_code();

  if (rc == base::HTTP_SC_INTERNAL_SERVER_ERROR || rc == base::HTTP_SC_SERVICE_UNAVAILABLE)
    return -EAGAIN; // temporary failure
  else if (rc != base::HTTP_SC_PARTIAL_CONTENT)
    return -EIO;

  return write_chunk(&req->get_output_buffer()[0], req->get_output_buffer().size(), part->offset);
}

int file::upload(const request::ptr & /* ignored */)
{
  int r;
  string returned_etag;

  r = prepare_upload();

  if (r)
    return r;

  if (service::is_multipart_upload_supported() && get_transfer_size() > config::get_upload_chunk_size())
    r = upload_multi(&returned_etag);
  else
    r = pool::call(threads::PR_REQ_0, bind(&file::upload_single, shared_from_this(), _1, &returned_etag));

  if (r)
    return r;

  r = finalize_upload(returned_etag);

  return r ? r : commit();
}

int file::prepare_upload()
{
  _hash_list.reset(new hash_list<sha256>(get_transfer_size()));

  return 0;
}

int file::finalize_upload(const string &returned_etag)
{
  set_etag(returned_etag);
  set_sha256_hash(_hash_list->get_root_hash<hex>());

  return 0;
}

int file::upload_single(const request::ptr &req, string *returned_etag)
{
  int r = 0;
  vector<char> buffer;
  string expected_md5_b64, expected_md5_hex, etag;
  uint8_t read_hash[md5::HASH_LEN];

  r = read_chunk(get_transfer_size(), 0, &buffer);

  if (r)
    return r;

  hash::compute<md5>(buffer, read_hash);

  expected_md5_b64 = encoder::encode<base64>(read_hash, md5::HASH_LEN);
  expected_md5_hex = encoder::encode<hex_with_quotes>(read_hash, md5::HASH_LEN);

  req->init(base::HTTP_PUT);
  req->set_url(get_url());

  set_request_headers(req);

  req->set_header("Content-MD5", expected_md5_b64);
  req->set_input_buffer(&buffer[0], buffer.size());

  req->run(config::get_transfer_timeout_in_s());

  if (req->get_response_code() != base::HTTP_SC_OK) {
    S3_LOG(LOG_WARNING, "file::upload_single", "failed to upload for [%s].\n", get_url().c_str());
    return -EIO;
  }

  etag = req->get_response_header("ETag");

  if (md5::is_valid_quoted_hex_hash(etag) && etag != expected_md5_hex) {
    S3_LOG(LOG_WARNING, "file::upload_single", "etag [%s] does not match md5 [%s].\n", etag.c_str(), expected_md5_hex.c_str());
    return -EIO;
  }
  
  *returned_etag = etag;

  return 0;
}

int file::upload_multi_init(const request::ptr &req, string *upload_id)
{
  xml::document doc;
  int r;

  req->init(base::HTTP_POST);
  req->set_url(get_url() + "?uploads");

  set_request_headers(req);

  req->run();

  if (req->get_response_code() != base::HTTP_SC_OK)
    return -EIO;

  doc = xml::parse(req->get_output_string());

  if (!doc) {
    S3_LOG(LOG_WARNING, "file::upload_multi_init", "failed to parse response.\n");
    return -EIO;
  }

  if ((r = xml::find(doc, UPLOAD_ID_XPATH, upload_id)))
    return r;

  if (upload_id->empty())
    return -EIO;

  return r;
}

int file::upload_multi_cancel(const request::ptr &req, const string &upload_id)
{
  S3_LOG(LOG_WARNING, "file::upload_multi_cancel", "one or more parts failed to upload for [%s].\n", get_url().c_str());

  req->init(base::HTTP_DELETE);
  req->set_url(get_url() + "?uploadId=" + upload_id);

  req->run();

  return 0;
}

int file::upload_multi_complete(const request::ptr &req, const string &upload_id, const string &upload_metadata, string *etag)
{
  xml::document doc;
  int r;

  req->init(base::HTTP_POST);
  req->set_url(get_url() + "?uploadId=" + upload_id);
  req->set_input_buffer(upload_metadata);
  req->set_header("Content-Type", "");

  // use the transfer timeout because completing a multi-part upload can take a long time
  // see http://docs.amazonwebservices.com/AmazonS3/latest/API/index.html?mpUploadComplete.html
  req->run(config::get_transfer_timeout_in_s());

  if (req->get_response_code() != base::HTTP_SC_OK) {
    S3_LOG(LOG_WARNING, "file::upload_multi_complete", "failed to complete multipart upload for [%s] with error %li.\n", get_url().c_str(), req->get_response_code());
    return -EIO;
  }

  doc = xml::parse(req->get_output_string());

  if (!doc) {
    S3_LOG(LOG_WARNING, "file::upload_multi_complete", "failed to parse response.\n");
    return -EIO;
  }

  if ((r = xml::find(doc, ETAG_XPATH, etag)))
    return r;

  if (etag->empty()) {
    S3_LOG(LOG_WARNING, "file::upload_multi_complete", "no etag on multipart upload of [%s]. response: %s\n", get_url().c_str(), req->get_output_string().c_str());
    return -EIO;
  }

  return 0;
}

int file::upload_multi(string *returned_etag)
{
  string upload_id, complete_upload;
  bool success = true;
  size_t size = get_transfer_size();
  size_t num_parts = (size + config::get_upload_chunk_size() - 1) / config::get_upload_chunk_size();
  size_t last_part = 0;
  vector<transfer_part> parts(num_parts);
  list<transfer_part *> parts_in_progress;
  int r;

  r = pool::call(threads::PR_REQ_0, bind(&file::upload_multi_init, shared_from_this(), _1, &upload_id));

  if (r)
    return r;

  for (size_t i = 0; i < num_parts; i++) {
    transfer_part *part = &parts[i];

    part->id = i;
    part->offset = i * config::get_upload_chunk_size();
    part->size = (i != num_parts - 1) ? config::get_upload_chunk_size() : (size - config::get_upload_chunk_size() * i);
  }

  for (last_part = 0; last_part < std::min(MAX_PARTS_IN_PROGRESS, num_parts); last_part++) {
    transfer_part *part = &parts[last_part];

    part->handle = pool::post(
      threads::PR_REQ_1, 
      bind(&file::upload_part, shared_from_this(), _1, upload_id, part));

    parts_in_progress.push_back(part);
  }

  while (!parts_in_progress.empty()) {
    transfer_part *part = parts_in_progress.front();
    int part_r;

    parts_in_progress.pop_front();
    part_r = part->handle->wait();

    if (part_r != 0)
      S3_LOG(LOG_DEBUG, "file::upload_multi", "part %i returned status %i for [%s].\n", part->id, part_r, get_url().c_str());

    // the default action is to not retry the failed part, and leave it with success = false

    if (part_r == 0) {
      part->success = true;

      if (last_part < num_parts) {
        part = &parts[last_part++];

        part->handle = pool::post(
          threads::PR_REQ_1, 
          bind(&file::upload_part, shared_from_this(), _1, upload_id, part));

        parts_in_progress.push_back(part);
      }

    } else if (part_r == -EAGAIN || part_r == -ETIMEDOUT) {
      part->retry_count++;

      if (part->retry_count <= config::get_max_transfer_retries()) {
        part->handle = pool::post(
          threads::PR_REQ_1, 
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
    pool::call(
      threads::PR_REQ_0, 
      bind(&file::upload_multi_cancel, shared_from_this(), _1, upload_id));

    return -EIO;
  }

  return pool::call(
    threads::PR_REQ_0, 
    bind(&file::upload_multi_complete, shared_from_this(), _1, upload_id, complete_upload, returned_etag));
}

int file::upload_part(const request::ptr &req, const string &upload_id, transfer_part *part)
{
  int r = 0;
  long rc = 0;
  vector<char> buffer;

  r = read_chunk(part->size, part->offset, &buffer);

  if (r)
    return r;

  part->etag = hash::compute<md5, hex_with_quotes>(buffer);

  req->init(base::HTTP_PUT);

  // part numbers are 1-based
  req->set_url(get_url() + "?partNumber=" + lexical_cast<string>(part->id + 1) + "&uploadId=" + upload_id);
  req->set_input_buffer(&buffer[0], buffer.size());

  req->run(config::get_transfer_timeout_in_s());
  rc = req->get_response_code();

  if (rc == base::HTTP_SC_INTERNAL_SERVER_ERROR || rc == base::HTTP_SC_SERVICE_UNAVAILABLE)
    return -EAGAIN; // temporary failure
  else if (rc != base::HTTP_SC_OK)
    return -EIO;

  if (req->get_response_header("ETag") != part->etag) {
    S3_LOG(LOG_WARNING, "file::upload_part", "md5 mismatch. expected %s, got %s.\n", part->etag.c_str(), req->get_response_header("ETag").c_str());
    return -EAGAIN; // assume it's a temporary failure
  }

  return 0;
}
