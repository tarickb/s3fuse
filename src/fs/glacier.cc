/*
 * fs/glacier.cc
 * -------------------------------------------------------------------------
 * Glacier auto-archive support class implementation.
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

#include "fs/glacier.h"

#include <string>

#include "base/logger.h"
#include "base/request.h"
#include "base/url.h"
#include "base/xml.h"
#include "fs/callback_xattr.h"
#include "services/service.h"
#include "threads/pool.h"

namespace s3 {
namespace fs {

namespace {
constexpr char STORAGE_CLASS_XPATH[] =
    "/ListBucketResult/Contents/StorageClass";

std::string ExtractQuotedPortion(const std::string &s) {
  if (s.empty()) return "";

  size_t pos = s.find('=');
  if (pos == std::string::npos) {
    S3_LOG(LOG_WARNING, "ExtractQuotedPortion", "malformed string: [%s]\n",
           s.c_str());
    return "";
  }

  std::string r = s.substr(pos + 1);
  if (*r.begin() != '"' || *(r.end() - 1) != '"') {
    S3_LOG(LOG_WARNING, "ExtractQuotedPortion", "missing quotes: [%s]\n",
           s.c_str());
    return "";
  }

  return r.substr(1, r.size() - 2);
}
}  // namespace

std::list<std::unique_ptr<XAttr>> Glacier::BuildXAttrs() {
  std::list<std::unique_ptr<XAttr>> xattrs;

  xattrs.push_back(CallbackXAttr::Create(
      PACKAGE_NAME "_storage_class",
      [this](std::string *out) {
        if (storage_class_.empty()) {
          int r = threads::Pool::Call(threads::PoolId::PR_REQ_1,
                                      std::bind(&Glacier::QueryStorageClass,
                                                this, std::placeholders::_1));
          if (r) return r;
        }
        *out = storage_class_;
        return 0;
      },
      [](std::string) { return 0; }, XAttr::XM_VISIBLE));

  xattrs.push_back(CallbackXAttr::Create(PACKAGE_NAME "restore_ongoing_",
                                         [this](std::string *out) {
                                           *out = restore_ongoing_;
                                           return 0;
                                         },
                                         [](std::string) { return 0; },
                                         XAttr::XM_VISIBLE));

  xattrs.push_back(CallbackXAttr::Create(PACKAGE_NAME "restore_expiry_",
                                         [this](std::string *out) {
                                           *out = restore_expiry_;
                                           return 0;
                                         },
                                         [](std::string) { return 0; },
                                         XAttr::XM_VISIBLE));

  xattrs.push_back(CallbackXAttr::Create(
      PACKAGE_NAME "_request_restore",
      [](std::string *out) {
        *out = "Set this attribute to N to restore for N days.";
        return 0;
      },
      [this](std::string in) {
        int days;
        try {
          days = std::stoi(in);
        } catch (...) {
          return -EINVAL;
        }
        return threads::Pool::Call(threads::PoolId::PR_REQ_1,
                                   std::bind(&Glacier::StartRestore, this,
                                             std::placeholders::_1, days));
      },
      XAttr::XM_VISIBLE | XAttr::XM_WRITABLE));

  return xattrs;
}

void Glacier::ExtractRestoreStatus(base::Request *req) {
  const std::string restore = req->response_header("x-amz-restore");

  restore_ongoing_.clear();
  restore_expiry_.clear();

  if (!restore.empty()) {
    size_t pos = restore.find(',');
    if (pos == std::string::npos) {
      restore_ongoing_ = restore;
    } else {
      restore_ongoing_ = restore.substr(0, pos);
      restore_expiry_ = restore.substr(pos + 2);
    }

    restore_ongoing_ = ExtractQuotedPortion(restore_ongoing_);
    restore_expiry_ = ExtractQuotedPortion(restore_expiry_);

    if (restore_ongoing_.empty()) {
      S3_LOG(LOG_WARNING, "Glacier::ExtractRestoreStatus",
             "malformed ongoing status string: [%s]\n", restore.c_str());
      restore_ongoing_ = "(error)";
    } else if (restore_ongoing_ == "false" && restore_expiry_.empty()) {
      S3_LOG(LOG_WARNING, "Glacier::ExtractRestoreStatus",
             "empty expiry when ongoing is false: [%s]\n", restore.c_str());
      restore_ongoing_ = "(error)";
    }
  }
}

int Glacier::QueryStorageClass(base::Request *req) {
  req->Init(base::HttpMethod::GET);
  req->SetUrl(services::Service::bucket_url(),
              std::string("max-keys=1&prefix=") + base::Url::Encode(path_));
  req->Run();
  if (req->response_code() != base::HTTP_SC_OK) return -EIO;

  auto doc = base::XmlDocument::Parse(req->GetOutputAsString());
  if (!doc) {
    S3_LOG(LOG_WARNING, "Glacier::QueryStorageClass",
           "failed to parse response.\n");
    return -EIO;
  }

  doc->Find(STORAGE_CLASS_XPATH, &storage_class_);
  if (storage_class_.empty()) {
    S3_LOG(LOG_WARNING, "Glacier::QueryStorageClass",
           "cannot find storage class.\n");
    return -EIO;
  }

  return 0;
}

int Glacier::StartRestore(base::Request *req, int days) {
  if (restore_ongoing_ == "true") {
    S3_LOG(LOG_DEBUG, "Glacier::StartRestore",
           "attempted to start restore when restore is ongoing on [%s]\n",
           path_.c_str());
    return 0;
  }

  req->Init(base::HttpMethod::POST);
  req->SetUrl(url_ + "?restore");
  req->SetHeader("Content-Type", "");

  const auto request_body = std::string("<RestoreRequest><Days>") +
                            std::to_string(days) + "</Days></RestoreRequest>";
  req->SetInputBuffer(request_body);

  req->Run();

  if (req->response_code() != base::HTTP_SC_OK &&
      req->response_code() != base::HTTP_SC_ACCEPTED) {
    S3_LOG(LOG_WARNING, "Glacier::StartRestore",
           "restore request failed for [%s] with status %i\n", path_.c_str(),
           req->response_code());
    return -EIO;
  }

  req->Init(base::HttpMethod::HEAD);
  req->SetUrl(url_);
  req->Run();

  if (req->response_code() != base::HTTP_SC_OK) {
    S3_LOG(LOG_WARNING, "Glacier::StartRestore",
           "failed to retrieve object metadata for [%s] with status %i\n",
           path_.c_str(), req->response_code());
    return -EIO;
  }

  ExtractRestoreStatus(req);
  return 0;
}

}  // namespace fs
}  // namespace s3
