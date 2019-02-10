/*
 * fs/symlink.cc
 * -------------------------------------------------------------------------
 * Symbolic link class implementation.
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

#include "fs/symlink.h"

#include "base/logger.h"
#include "base/request.h"

namespace s3 {
namespace fs {

namespace {
constexpr char CONTENT_TYPE[] = "text/symlink";
const std::string CONTENT_PREFIX = "SYMLINK:";

Object *Checker(const std::string &path, base::Request *req) {
  if (req->response_header("Content-Type") != CONTENT_TYPE) return nullptr;
  return new Symlink(path);
}

Object::TypeCheckers::Entry s_checker_reg(Checker, 100);
}  // namespace

Symlink::Symlink(const std::string &path) : Object(path) {
  set_content_type(CONTENT_TYPE);
  set_type(S_IFLNK);
}

int Symlink::Read(std::string *target) {
  std::unique_lock<std::mutex> lock(mutex_);

  if (target_.empty()) {
    lock.unlock();
    int r = threads::Pool::Call(
        threads::PoolId::PR_REQ_0,
        std::bind(&Symlink::DoRead, this, std::placeholders::_1));
    if (r) return r;
    lock.lock();
  }

  *target = target_;
  return 0;
}

void Symlink::SetTarget(const std::string &target) {
  std::lock_guard<std::mutex> lock(mutex_);
  target_ = target;
}

void Symlink::SetRequestBody(base::Request *req) {
  std::lock_guard<std::mutex> lock(mutex_);
  req->SetInputBuffer(CONTENT_PREFIX + target_);
}

int Symlink::DoRead(base::Request *req) {
  req->Init(base::HttpMethod::GET);
  req->SetUrl(url());

  req->Run();
  if (req->response_code() != base::HTTP_SC_OK) return -EIO;

  std::string output = req->GetOutputAsString();
  if (output.substr(0, CONTENT_PREFIX.size()) != CONTENT_PREFIX) {
    S3_LOG(LOG_WARNING, "Symlink::DoRead",
           "content prefix does not match: [%s]\n", output.c_str());
    return -EINVAL;
  }

  std::lock_guard<std::mutex> lock(mutex_);
  target_ = output.substr(CONTENT_PREFIX.size());
  return 0;
}

}  // namespace fs
}  // namespace s3
