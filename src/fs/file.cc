/*
 * fs/file.cc
 * -------------------------------------------------------------------------
 * File class implementation.
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
#include <mutex>

#include "base/config.h"
#include "base/logger.h"
#include "base/request.h"
#include "base/statistics.h"
#include "crypto/hash.h"
#include "crypto/hex.h"
#include "crypto/hex_with_quotes.h"
#include "crypto/md5.h"
#include "fs/cache.h"
#include "fs/file.h"
#include "fs/metadata.h"
#include "fs/mime_types.h"
#include "fs/static_xattr.h"
#include "services/file_transfer.h"
#include "services/service.h"
#include "threads/pool.h"

#define TEMP_NAME_TEMPLATE "/tmp/s3fuse.local-XXXXXX"

namespace s3 {
namespace fs {

namespace {
const off_t TRUNCATE_LIMIT = 4ULL * 1024 * 1024 * 1024; // 4 GB

std::atomic_int s_sha256_mismatches(0), s_md5_mismatches(0),
    s_no_hash_checks(0);
std::atomic_int s_non_dirty_flushes(0), s_reopens(0);

object *checker(const std::string &path, const base::request::ptr &req) {
  return new file(path);
}

void statistics_writer(std::ostream *o) {
  *o << "files:\n"
        "  sha256 mismatches: "
     << s_sha256_mismatches << ", md5 mismatches: " << s_md5_mismatches
     << ", no hash checks: " << s_no_hash_checks
     << "\n"
        "  non-dirty flushes: "
     << s_non_dirty_flushes
     << "\n"
        "  reopens: "
     << s_reopens << "\n";
}

object::type_checker_list::entry s_checker_reg(checker, 1000);
base::statistics::writers::entry s_writer(statistics_writer, 0);
} // namespace

void file::test_transfer_chunk_sizes() {
  size_t chunk_size = crypto::hash_list<crypto::sha256>::CHUNK_SIZE;

  if (services::service::get_file_transfer()->get_download_chunk_size() %
      chunk_size) {
    S3_LOG(LOG_ERR, "file::test_transfer_chunk_size",
           "download chunk size must be a multiple of %zu.\n", chunk_size);
    throw std::runtime_error("invalid download chunk size");
  }

  if (services::service::get_file_transfer()->get_upload_chunk_size() %
      chunk_size) {
    S3_LOG(LOG_ERR, "file::test_transfer_chunk_size",
           "upload chunk size must be a multiple of %zu.\n", chunk_size);
    throw std::runtime_error("invalid upload chunk size");
  }
}

void file::open_locked_object(const object::ptr &obj, file_open_mode mode,
                              uint64_t *handle, int *status) {
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

int file::open(const std::string &path, file_open_mode mode, uint64_t *handle) {
  int r = -EINVAL;

  cache::lock_object(path, bind(&file::open_locked_object,
                                std::placeholders::_1, mode, handle, &r));

  return r;
}

file::file(const std::string &path)
    : object(path), _fd(-1), _status(0), _async_error(0), _read_only(false),
      _ref_count(0) {
  set_type(S_IFREG);

  if (base::config::get_auto_detect_mime_type()) {
    size_t pos = path.find_last_of('.');

    if (pos != std::string::npos) {
      std::string ext = path.substr(pos + 1);
      std::string content_type = mime_types::get_type_by_extension(ext);

      if (!content_type.empty())
        set_content_type(content_type);
    }
  }
}

file::~file() {}

bool file::is_removable() {
  std::lock_guard<std::mutex> lock(_fs_mutex);

  return _ref_count == 0 && object::is_removable();
}

void file::init(const base::request::ptr &req) {
  const std::string &meta_prefix = services::service::get_header_meta_prefix();

  object::init(req);

  if (is_intact()) {
    // we were the last people to modify this object, so everything should be
    // as we left it

    set_sha256_hash(req->get_response_header(meta_prefix + metadata::SHA256));
  }
}

void file::set_sha256_hash(const std::string &hash) {
  if (hash.empty())
    return;

  _sha256_hash = hash;
  get_metadata()->replace(static_xattr::from_string(PACKAGE_NAME "_sha256",
                                                    hash, xattr::XM_VISIBLE));
}

void file::set_request_headers(const base::request::ptr &req) {
  const std::string &meta_prefix = services::service::get_header_meta_prefix();

  object::set_request_headers(req);

  req->set_header(meta_prefix + metadata::SHA256, _sha256_hash);
}

void file::on_download_complete(int ret) {
  std::lock_guard<std::mutex> lock(_fs_mutex);

  if (_status != FS_DOWNLOADING) {
    S3_LOG(LOG_ERR, "file::download_complete",
           "inconsistent state for [%s]. don't know what to do.\n",
           get_path().c_str());
    return;
  }

  _async_error = ret;
  _status = 0;
  _condition.notify_all();
}

int file::is_downloadable() { return 0; }

int file::open(file_open_mode mode, uint64_t *handle) {
  std::lock_guard<std::mutex> lock(_fs_mutex);

  if (_ref_count == 0) {
    char temp_name[] = TEMP_NAME_TEMPLATE;
    off_t size = get_stat()->st_size;

    _fd = mkstemp(temp_name);
    unlink(temp_name);

    S3_LOG(LOG_DEBUG, "file::open", "opening [%s] in [%s].\n",
           get_path().c_str(), temp_name);

    if (_fd == -1)
      return -errno;

    if (object::is_versioned_path(get_path()))
      _read_only = true;

    if (mode & fs::OPEN_TRUNCATE_TO_ZERO) {
      if (_read_only)
        return -EROFS;

      // if the file had a non-zero size but was opened with O_TRUNC, we need
      // to write back a zero-length file.
      if (size)
        _status = FS_DIRTY;

    } else {
      if (ftruncate(_fd, size) != 0)
        return -errno;

      if (size > 0) {
        int r;

        r = is_downloadable();

        if (r)
          return r;

        _status = FS_DOWNLOADING;

        threads::pool::post(
            threads::PR_0,
            bind(&file::download, shared_from_this(), std::placeholders::_1),
            bind(&file::on_download_complete, shared_from_this(),
                 std::placeholders::_1));
      }
    }
  } else {
    ++s_reopens;
  }

  *handle = reinterpret_cast<uint64_t>(this);
  _ref_count++;

  return 0;
}

int file::release() {
  std::lock_guard<std::mutex> lock(_fs_mutex);

  if (_ref_count == 0) {
    S3_LOG(LOG_WARNING, "file::release",
           "attempt to release file [%s] with zero ref-count\n",
           get_path().c_str());
    return -EINVAL;
  }

  _ref_count--;

  if (_ref_count == 0) {
    if (_status != 0) {
      S3_LOG(LOG_ERR, "file::release",
             "released file [%s] with non-quiescent status [%i].\n",
             get_path().c_str(), _status);
      return -EBUSY;
    }

    // update stat here so that subsequent calls to copy_stat() will get the
    // correct file size
    update_stat(lock);

    close(_fd);
    _fd = -1;

    expire();
  }

  return 0;
}

int file::flush() {
  std::unique_lock<std::mutex> lock(_fs_mutex);

  while (_status & (FS_DOWNLOADING | FS_UPLOADING | FS_WRITING))
    _condition.wait(lock);

  if (_async_error)
    return _async_error;

  if (!(_status & FS_DIRTY)) {
    ++s_non_dirty_flushes;

    S3_LOG(LOG_DEBUG, "file::flush",
           "skipping flush for non-dirty file [%s].\n", get_path().c_str());
    return 0;
  }

  _status |= FS_UPLOADING;

  lock.unlock();
  _async_error =
      threads::pool::call(threads::PR_0, bind(&file::upload, shared_from_this(),
                                              std::placeholders::_1));
  lock.lock();

  _status = 0;
  _condition.notify_all();

  return _async_error;
}

int file::write(const char *buffer, size_t size, off_t offset) {
  std::unique_lock<std::mutex> lock(_fs_mutex);
  int r;

  if (_read_only)
    return -EROFS;

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

  return (r < 0) ? -errno : r;
}

int file::read(char *buffer, size_t size, off_t offset) {
  std::unique_lock<std::mutex> lock(_fs_mutex);
  int r;

  while (_status & FS_DOWNLOADING)
    _condition.wait(lock);

  if (_async_error)
    return _async_error;

  lock.unlock();
  r = pread(_fd, buffer, size, offset);

  return (r < 0) ? -errno : r;
}

int file::truncate(off_t length) {
  std::unique_lock<std::mutex> lock(_fs_mutex);
  int r;

  if (length > TRUNCATE_LIMIT)
    return -EINVAL;

  if (_read_only)
    return -EROFS;

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

int file::write_chunk(const char *buffer, size_t size, off_t offset) {
  ssize_t r;

  r = pwrite(_fd, buffer, size, offset);

  if (r != static_cast<ssize_t>(size))
    return -errno;

  if (_hash_list)
    _hash_list->compute_hash(offset, reinterpret_cast<const uint8_t *>(buffer),
                             size);

  return 0;
}

int file::read_chunk(size_t size, off_t offset,
                     const base::char_vector_ptr &buffer) {
  ssize_t r;

  buffer->resize(size);
  r = pread(_fd, &(*buffer)[0], size, offset);

  if (r != static_cast<ssize_t>(size))
    return -errno;

  if (_hash_list)
    _hash_list->compute_hash(
        offset, reinterpret_cast<const uint8_t *>(&(*buffer)[0]), size);

  return 0;
}

size_t file::get_local_size() {
  struct stat s;

  if (fstat(_fd, &s) == -1) {
    S3_LOG(LOG_WARNING, "file::get_local_size", "failed to stat [%s].\n",
           get_path().c_str());

    throw std::runtime_error("failed to stat local file");
  }

  return s.st_size;
}

void file::update_stat() {
  object::update_stat();

  {
    std::lock_guard<std::mutex> lock(_fs_mutex);

    update_stat(lock);
  }
}

void file::update_stat(const std::lock_guard<std::mutex> &) {
  if (_fd != -1)
    get_stat()->st_size = get_local_size();
}

int file::download(const base::request::ptr & /* ignored */) {
  int r = 0;

  r = prepare_download();

  if (r)
    return r;

  r = services::service::get_file_transfer()->download(
      get_url(), get_local_size(),
      bind(&file::write_chunk, shared_from_this(), std::placeholders::_1,
           std::placeholders::_2, std::placeholders::_3));

  if (r)
    return r;

  return finalize_download();
}

int file::prepare_download() {
  if (!_sha256_hash.empty())
    _hash_list.reset(new crypto::hash_list<crypto::sha256>(get_local_size()));

  return 0;
}

int file::finalize_download() {
  if (!_sha256_hash.empty()) {
    std::string computed_hash = _hash_list->get_root_hash<crypto::hex>();

    if (computed_hash != _sha256_hash) {
      ++s_sha256_mismatches;

      S3_LOG(LOG_WARNING, "file::finalize_download",
             "sha256 mismatch for %s. expected %s, got %s.\n",
             get_path().c_str(), _sha256_hash.c_str(), computed_hash.c_str());

      return -EIO;
    }
  } else if (crypto::md5::is_valid_quoted_hex_hash(get_etag())) {
    // as a fallback, use the etag as an md5 hash of the file
    std::string computed_hash =
        crypto::hash::compute<crypto::md5, crypto::hex_with_quotes>(_fd);

    if (computed_hash != get_etag()) {
      ++s_md5_mismatches;

      S3_LOG(LOG_WARNING, "file::finalize_download",
             "md5 mismatch for %s. expected %s, got %s.\n", get_path().c_str(),
             computed_hash.c_str(), get_etag().c_str());

      return -EIO;
    }
  } else {
    ++s_no_hash_checks;

    S3_LOG(LOG_WARNING, "file::finalize_download",
           "no hash check performed for %s\n", get_path().c_str());
  }

  return 0;
}

int file::upload(const base::request::ptr & /* ignored */) {
  int r;
  std::string returned_etag;

  r = prepare_upload();

  if (r)
    return r;

  r = services::service::get_file_transfer()->upload(
      get_url(), get_local_size(),
      bind(&file::read_chunk, shared_from_this(), std::placeholders::_1,
           std::placeholders::_2, std::placeholders::_3),
      &returned_etag);

  if (r)
    return r;

  r = finalize_upload(returned_etag);

  return r ? r : commit();
}

int file::prepare_upload() {
  _hash_list.reset(new crypto::hash_list<crypto::sha256>(get_local_size()));

  return 0;
}

int file::finalize_upload(const std::string &returned_etag) {
  set_etag(returned_etag);
  set_sha256_hash(_hash_list->get_root_hash<crypto::hex>());

  return 0;
}
} // namespace fs
} // namespace s3
