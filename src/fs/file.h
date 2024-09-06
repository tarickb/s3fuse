/*
 * fs/file.h
 * -------------------------------------------------------------------------
 * Represents a file object.
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

#ifndef S3_FS_FILE_H
#define S3_FS_FILE_H

#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "base/request.h"
#include "crypto/hash_list.h"
#include "crypto/sha256.h"
#include "fs/object.h"

namespace s3 {
namespace fs {
enum class FileOpenMode { DEFAULT, TRUNCATE_TO_ZERO };

class File : public Object {
 public:
  inline static File *FromHandle(uint64_t handle) {
    return reinterpret_cast<File *>(handle);
  }

  static void TestTransferChunkSizes();

  static int Open(const std::string &path, FileOpenMode mode, uint64_t *handle);

  explicit File(const std::string &path);
  ~File() override = default;

  bool IsRemovable() override;

  int Release();
  int Flush();
  int Write(const char *buffer, size_t size, off_t offset);
  int Read(char *buffer, size_t size, off_t offset);
  int Truncate(off_t length);

 protected:
  void Init(base::Request *req) override;
  void SetRequestHeaders(base::Request *req) override;
  void UpdateStat() override;

  virtual int IsDownloadable();

  virtual int WriteChunk(const char *buffer, size_t size, off_t offset);
  virtual int ReadChunk(size_t size, off_t offset, std::vector<char> *buffer);

  virtual int PrepareDownload();
  virtual int FinalizeDownload();

  virtual int PrepareUpload();
  virtual int FinalizeUpload(const std::string &returned_etag);

  inline std::string sha256_hash() { return sha256_hash_; }
  void SetSha256Hash(const std::string &hash);

 private:
  enum Status {
    FS_DOWNLOADING = 0x1,
    FS_UPLOADING = 0x2,
    FS_WRITING = 0x4,
    FS_DIRTY = 0x8
  };

  int Open(FileOpenMode mode, uint64_t *handle);

  int Download(base::Request *);
  void OnDownloadComplete(int ret);
  int Upload(base::Request *);

  size_t GetLocalSize();

  void UpdateStat(const std::lock_guard<std::mutex> &);

  std::mutex fs_mutex_;
  std::condition_variable condition_;
  std::unique_ptr<crypto::HashList<crypto::Sha256>> hash_list_;
  std::string sha256_hash_;

  // protected by fs_mutex_
  int fd_ = -1, status_ = 0, async_error_ = 0;
  bool read_only_ = false;
  uint64_t ref_count_ = 0;
};
}  // namespace fs
}  // namespace s3

#endif
