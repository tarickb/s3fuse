/*
 * base/request.h
 * -------------------------------------------------------------------------
 * HTTP request.
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

#ifndef S3_BASE_REQUEST_H
#define S3_BASE_REQUEST_H

#include <stdio.h>

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace s3 {
namespace base {
enum class HttpMethod { INVALID, DELETE, GET, HEAD, POST, PUT };

enum HttpStatusCode {
  HTTP_SC_OK = 200,
  HTTP_SC_CREATED = 201,
  HTTP_SC_ACCEPTED = 202,
  HTTP_SC_NO_CONTENT = 204,
  HTTP_SC_PARTIAL_CONTENT = 206,
  HTTP_SC_MULTIPLE_CHOICES = 300,
  HTTP_SC_RESUME = 308,
  HTTP_SC_BAD_REQUEST = 400,
  HTTP_SC_UNAUTHORIZED = 401,
  HTTP_SC_FORBIDDEN = 403,
  HTTP_SC_NOT_FOUND = 404,
  HTTP_SC_PRECONDITION_FAILED = 412,
  HTTP_SC_INTERNAL_SERVER_ERROR = 500,
  HTTP_SC_SERVICE_UNAVAILABLE = 503
};

const char *HttpMethodToString(HttpMethod method);

class Request;
class RequestHook;
class Transport;

class RequestFactory {
 public:
  static void SetHook(RequestHook *hook);

  static std::unique_ptr<Request> New();
  static std::unique_ptr<Request> NewNoHook();
};

using HeaderMap = std::map<std::string, std::string>;

class Request {
 public:
  static constexpr int DEFAULT_REQUEST_TIMEOUT = -1;

  ~Request();

  void Init(HttpMethod method);

  inline HttpMethod method() const { return method_; }
  inline std::string url() const { return url_; }
  inline const HeaderMap &headers() const { return headers_; }
  inline const std::vector<char> &output_buffer() const {
    return output_buffer_;
  }
  inline std::string response_header(const std::string &key) const {
    auto iter = response_headers_.find(key);
    return (iter == response_headers_.end()) ? "" : iter->second;
  }
  inline const HeaderMap &response_headers() const { return response_headers_; }
  inline int response_code() const { return response_code_; }
  inline time_t last_modified() const { return last_modified_; }
  inline double current_run_time() const { return current_run_time_; }

  std::string GetOutputAsString() const;

  void SetUrl(const std::string &url, const std::string &query_string = "");
  void SetHeader(const std::string &name, const std::string &value);
  void SetInputBuffer(std::vector<char> &&buffer);
  void SetInputBuffer(const std::string &str);

  void ResetCurrentRunTime();

  void Run(int timeout_in_s = DEFAULT_REQUEST_TIMEOUT);

 private:
  friend class RequestFactory;  // for ctor.

  static size_t ProcessHeaderWrapper(char *data, size_t size, size_t items,
                                     void *context) {
    return static_cast<Request *>(context)->ProcessHeader(data, size, items);
  }

  static size_t WriteOutputWrapper(char *data, size_t size, size_t items,
                                   void *context) {
    return static_cast<Request *>(context)->WriteOutput(data, size, items);
  }

  static size_t ReadInputWrapper(char *data, size_t size, size_t items,
                                 void *context) {
    return static_cast<Request *>(context)->ReadInput(data, size, items);
  }

  static int SeekInputWrapper(void *context, off_t offset, int origin) {
    return static_cast<Request *>(context)->SeekInput(offset, origin);
  }

  static int ProgressWrapper(void *context, off_t dl_total, off_t dl_now,
                             off_t ul_total, off_t ul_now) {
    return static_cast<Request *>(context)->Progress(dl_total, dl_now, ul_total,
                                                     ul_now);
  }

  explicit Request(RequestHook *hook);

  size_t ProcessHeader(char *data, size_t size, size_t items);
  size_t WriteOutput(char *data, size_t size, size_t items);
  size_t ReadInput(char *data, size_t size, size_t items);
  int SeekInput(off_t offset, int origin);
  int Progress(off_t dl_total, off_t dl_now, off_t ul_total, off_t ul_now);

  void Rewind();

  // not reset by Init()
  const std::unique_ptr<Transport> transport_;
  RequestHook *const hook_ = nullptr;

  double current_run_time_ = 0.0, total_run_time_ = 0.0;
  uint64_t run_count_ = 0;
  uint64_t total_bytes_transferred_ = 0;
  time_t deadline_ = 0;

  // should be reset by Init()
  static constexpr int ERROR_MESSAGE_BUFFER_LEN = 256;
  char transport_error_[ERROR_MESSAGE_BUFFER_LEN];

  HttpMethod method_ = HttpMethod::INVALID;
  std::string url_, transport_url_;
  HeaderMap response_headers_;

  std::vector<char> output_buffer_;

  int response_code_ = 0;
  time_t last_modified_ = 0;

  // assumptions: no duplicates, all header names are always lower-case
  HeaderMap headers_;

  std::vector<char> input_buffer_;

  // reset only by Rewind()
  const char *input_pos_ = nullptr;
  size_t input_remaining_ = 0;
};
}  // namespace base
}  // namespace s3

#endif
