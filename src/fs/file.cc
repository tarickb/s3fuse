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

#include "fs/file.h"

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
#include "fs/metadata.h"
#include "fs/mime_types.h"
#include "fs/static_xattr.h"
#include "services/file_transfer.h"
#include "services/service.h"
#include "threads/pool.h"

#define TEMP_NAME_TEMPLATE "/tmp/" PACKAGE_NAME ".local-XXXXXX"

namespace s3 {
namespace fs {

namespace {
const off_t TRUNCATE_LIMIT = 4ULL * 1024 * 1024 * 1024;  // 4 GB

std::atomic_int s_sha256_mismatches(0), s_md5_mismatches(0),
    s_no_hash_checks(0);
std::atomic_int s_non_dirty_flushes(0), s_reopens(0);

Object *Checker(const std::string &path, base::Request *req) {
  return new File(path);
}

void StatsWriter(std::ostream *o) {
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

Object::TypeCheckers::Entry s_checker_reg(Checker, 1000);
base::Statistics::Writers::Entry s_writer(StatsWriter, 0);
}  // namespace

void File::TestTransferChunkSizes() {
  const size_t chunk_size = crypto::HashList<crypto::Sha256>::CHUNK_SIZE;
  if (services::Service::file_transfer()->download_chunk_size() % chunk_size) {
    S3_LOG(LOG_ERR, "File::TestTransferChunkSizes",
           "download chunk size must be a multiple of %zu.\n", chunk_size);
    throw std::runtime_error("invalid download chunk size");
  }
  if (services::Service::file_transfer()->upload_chunk_size() % chunk_size) {
    S3_LOG(LOG_ERR, "File::TestTransferChunkSizes",
           "upload chunk size must be a multiple of %zu.\n", chunk_size);
    throw std::runtime_error("invalid upload chunk size");
  }
}

int File::Open(const std::string &path, FileOpenMode mode, uint64_t *handle) {
  int status = -EINVAL;

  Cache::LockObject(path, [&status, mode, handle](std::shared_ptr<Object> obj) {
    if (!obj) {
      status = -ENOENT;
      return;
    }
    if (obj->type() != S_IFREG) {
      status = -EINVAL;
      return;
    }
    status = static_cast<File *>(obj.get())->Open(mode, handle);
  });

  return status;
}

File::File(const std::string &path) : Object(path) {
  set_type(S_IFREG);

  if (base::Config::auto_detect_mime_type()) {
    size_t pos = path.find_last_of('.');
    if (pos != std::string::npos) {
      std::string ext = path.substr(pos + 1);
      std::string content_type = MimeTypes::GetTypeByExtension(ext);
      if (!content_type.empty()) set_content_type(content_type);
    }
  }
}

bool File::IsRemovable() {
  std::lock_guard<std::mutex> lock(fs_mutex_);
  return ref_count_ == 0 && Object::IsRemovable();
}

int File::Release() {
  std::lock_guard<std::mutex> lock(fs_mutex_);

  if (ref_count_ == 0) {
    S3_LOG(LOG_WARNING, "File::Release",
           "attempt to release file [%s] with zero ref-count\n",
           path().c_str());
    return -EINVAL;
  }

  --ref_count_;
  if (ref_count_ == 0) {
    if (status_ != 0) {
      S3_LOG(LOG_ERR, "File::Release",
             "released file [%s] with non-quiescent status [%i].\n",
             path().c_str(), status_);
      return -EBUSY;
    }

    // update stat here so that subsequent calls to copy_stat() will get the
    // correct file size
    UpdateStat(lock);

    close(fd_);
    fd_ = -1;

    Expire();
  }

  return 0;
}

int File::Flush() {
  std::unique_lock<std::mutex> lock(fs_mutex_);

  while (status_ & (FS_DOWNLOADING | FS_UPLOADING | FS_WRITING))
    condition_.wait(lock);

  if (async_error_) return async_error_;

  if (!(status_ & FS_DIRTY)) {
    ++s_non_dirty_flushes;
    S3_LOG(LOG_DEBUG, "File::Flush",
           "skipping flush for non-dirty file [%s].\n", path().c_str());
    return 0;
  }

  status_ |= FS_UPLOADING;

  lock.unlock();
  async_error_ = threads::Pool::Call(
      threads::PoolId::PR_0, bind(&File::Upload, this, std::placeholders::_1));
  lock.lock();

  status_ = 0;
  condition_.notify_all();

  return async_error_;
}

int File::Write(const char *buffer, size_t size, off_t offset) {
  std::unique_lock<std::mutex> lock(fs_mutex_);

  if (read_only_) return -EROFS;

  while (status_ & (FS_DOWNLOADING | FS_UPLOADING)) condition_.wait(lock);

  if (async_error_) return async_error_;

  status_ |= FS_DIRTY | FS_WRITING;

  lock.unlock();
  int r = pwrite(fd_, buffer, size, offset);
  lock.lock();

  status_ &= ~FS_WRITING;
  condition_.notify_all();

  return (r < 0) ? -errno : r;
}

int File::Read(char *buffer, size_t size, off_t offset) {
  std::unique_lock<std::mutex> lock(fs_mutex_);

  while (status_ & FS_DOWNLOADING) condition_.wait(lock);

  if (async_error_) return async_error_;

  lock.unlock();
  int r = pread(fd_, buffer, size, offset);

  return (r < 0) ? -errno : r;
}

int File::Truncate(off_t length) {
  std::unique_lock<std::mutex> lock(fs_mutex_);

  if (length > TRUNCATE_LIMIT) return -EINVAL;
  if (read_only_) return -EROFS;

  while (status_ & (FS_DOWNLOADING | FS_UPLOADING)) condition_.wait(lock);

  if (async_error_) return async_error_;

  status_ |= FS_DIRTY | FS_WRITING;

  lock.unlock();
  int r = ftruncate(fd_, length);
  lock.lock();

  status_ &= ~FS_WRITING;
  condition_.notify_all();

  return r;
}

void File::Init(base::Request *req) {
  Object::Init(req);

  if (intact()) {
    // we were the last people to modify this object, so everything should be
    // as we left it
    SetSha256Hash(req->response_header(services::Service::header_meta_prefix() +
                                       Metadata::SHA256));
  }
}

void File::SetRequestHeaders(base::Request *req) {
  Object::SetRequestHeaders(req);
  req->SetHeader(services::Service::header_meta_prefix() + Metadata::SHA256,
                 sha256_hash_);
}

void File::UpdateStat() {
  Object::UpdateStat();
  {
    std::lock_guard<std::mutex> lock(fs_mutex_);
    UpdateStat(lock);
  }
}

int File::IsDownloadable() { return 0; }

int File::WriteChunk(const char *buffer, size_t size, off_t offset) {
  ssize_t r = pwrite(fd_, buffer, size, offset);
  if (r != static_cast<ssize_t>(size)) return -errno;
  if (hash_list_)
    hash_list_->ComputeHash(offset, reinterpret_cast<const uint8_t *>(buffer),
                            size);
  return 0;
}

int File::ReadChunk(size_t size, off_t offset, std::vector<char> *buffer) {
  buffer->resize(size);
  ssize_t r = pread(fd_, &(*buffer)[0], size, offset);
  if (r != static_cast<ssize_t>(size)) return -errno;
  if (hash_list_)
    hash_list_->ComputeHash(
        offset, reinterpret_cast<const uint8_t *>(&(*buffer)[0]), size);
  return 0;
}

int File::PrepareDownload() {
  if (!sha256_hash_.empty())
    hash_list_.reset(new crypto::HashList<crypto::Sha256>(GetLocalSize()));
  return 0;
}

int File::FinalizeDownload() {
  if (!sha256_hash_.empty()) {
    std::string computed_hash = hash_list_->GetRootHash<crypto::Hex>();
    if (computed_hash != sha256_hash_) {
      ++s_sha256_mismatches;
      S3_LOG(LOG_WARNING, "File::FinalizeDownload",
             "sha256 mismatch for %s. expected %s, got %s.\n", path().c_str(),
             sha256_hash_.c_str(), computed_hash.c_str());
      return -EIO;
    }
  } else if (crypto::Md5::IsValidQuotedHexHash(etag())) {
    // as a fallback, use the etag as an md5 hash of the file
    std::string computed_hash =
        crypto::Hash::Compute<crypto::Md5, crypto::HexWithQuotes>(fd_);
    if (computed_hash != etag()) {
      ++s_md5_mismatches;
      S3_LOG(LOG_WARNING, "File::FinalizeDownload",
             "md5 mismatch for %s. expected %s, got %s.\n", path().c_str(),
             computed_hash.c_str(), etag().c_str());
      return -EIO;
    }
  } else {
    ++s_no_hash_checks;
    S3_LOG(LOG_WARNING, "File::FinalizeDownload",
           "no hash check performed for %s\n", path().c_str());
  }
  return 0;
}

int File::PrepareUpload() {
  hash_list_.reset(new crypto::HashList<crypto::Sha256>(GetLocalSize()));
  return 0;
}

int File::FinalizeUpload(const std::string &returned_etag) {
  set_etag(returned_etag);
  SetSha256Hash(hash_list_->GetRootHash<crypto::Hex>());
  return 0;
}

void File::SetSha256Hash(const std::string &hash) {
  if (hash.empty()) return;
  sha256_hash_ = hash;
  UpdateMetadata(
      StaticXAttr::FromString(PACKAGE_NAME "_sha256", hash, XAttr::XM_VISIBLE));
}

int File::Open(FileOpenMode mode, uint64_t *handle) {
  std::lock_guard<std::mutex> lock(fs_mutex_);

  if (ref_count_ == 0) {
    char temp_name[] = TEMP_NAME_TEMPLATE;
    const off_t size = stat()->st_size;

    fd_ = mkstemp(temp_name);
    unlink(temp_name);

    S3_LOG(LOG_DEBUG, "File::Open", "opening [%s] in [%s].\n", path().c_str(),
           temp_name);

    if (fd_ == -1) return -errno;

    if (Object::IsVersionedPath(path())) read_only_ = true;

    if (mode == FileOpenMode::TRUNCATE_TO_ZERO) {
      if (read_only_) return -EROFS;
      // if the file had a non-zero size but was opened with O_TRUNC, we need
      // to write back a zero-length file.
      if (size) status_ = FS_DIRTY;
    } else {
      if (ftruncate(fd_, size) != 0) return -errno;
      if (size > 0) {
        int r = IsDownloadable();
        if (r) return r;
        status_ = FS_DOWNLOADING;
        threads::Pool::Post(
            threads::PoolId::PR_0,
            bind(&File::Download, this, std::placeholders::_1),
            bind(&File::OnDownloadComplete, this, std::placeholders::_1));
      }
    }
  } else {
    ++s_reopens;
  }

  *handle = reinterpret_cast<uint64_t>(this);
  ref_count_++;

  return 0;
}

int File::Download(base::Request * /* ignored */) {
  int r = PrepareDownload();
  if (r) return r;

  r = services::Service::file_transfer()->Download(
      url(), GetLocalSize(),
      bind(&File::WriteChunk, this, std::placeholders::_1,
           std::placeholders::_2, std::placeholders::_3));
  if (r) return r;

  return FinalizeDownload();
}

void File::OnDownloadComplete(int ret) {
  std::lock_guard<std::mutex> lock(fs_mutex_);

  if (status_ != FS_DOWNLOADING) {
    S3_LOG(LOG_ERR, "File::OnDownloadComplete",
           "inconsistent state for [%s]. don't know what to do.\n",
           path().c_str());
    return;
  }

  async_error_ = ret;
  status_ = 0;
  condition_.notify_all();
}

int File::Upload(base::Request * /* ignored */) {
  int r = PrepareUpload();
  if (r) return r;
  std::string returned_etag;
  r = services::Service::file_transfer()->Upload(
      url(), GetLocalSize(),
      bind(&File::ReadChunk, this, std::placeholders::_1, std::placeholders::_2,
           std::placeholders::_3),
      &returned_etag);
  if (r) return r;
  r = FinalizeUpload(returned_etag);
  return r ? r : Commit();
}

size_t File::GetLocalSize() {
  struct stat s;
  if (fstat(fd_, &s) == -1) {
    S3_LOG(LOG_WARNING, "File::GetLocalSize", "failed to stat [%s].\n",
           path().c_str());
    throw std::runtime_error("failed to stat local file");
  }
  return s.st_size;
}

void File::UpdateStat(const std::lock_guard<std::mutex> &) {
  if (fd_ != -1) stat()->st_size = GetLocalSize();
}

}  // namespace fs
}  // namespace s3
