/*
 * base/request.cc
 * -------------------------------------------------------------------------
 * Executes HTTP requests using libcurl.
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

#include "base/request.h"

#include <curl/curl.h>
#include <string.h>

#ifdef HAVE_GNUTLS
#include <gnutls/gnutls.h>
#endif

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <iomanip>
#include <mutex>
#include <stdexcept>

#include "base/config.h"
#include "base/logger.h"
#include "base/request_hook.h"
#include "base/statistics.h"
#include "base/timer.h"

#define TEST_OK(x)                                                           \
  do {                                                                       \
    if ((x) != CURLE_OK) throw std::runtime_error("call to " #x " failed."); \
  } while (0)

namespace s3 {
namespace base {

namespace {
constexpr char USER_AGENT[] = PACKAGE_NAME " " PACKAGE_VERSION_WITH_REV;

uint64_t s_run_count = 0;
uint64_t s_total_bytes = 0;
double s_run_time = 0.0;

std::atomic_int s_curl_failures(0), s_request_failures(0);
std::atomic_int s_timeouts(0), s_aborts(0), s_hook_retries(0);
std::atomic_int s_rewinds(0);
std::mutex s_stats_mutex;

class HttpMethodCounters {
 public:
  HttpMethodCounters() = default;

  void Increment(s3::base::HttpMethod method) {
    std::lock_guard<std::mutex> lock(mutex_);
    ++counters_[method];
  }

  void Write(std::ostream *o) {
    std::lock_guard<std::mutex> lock(mutex_);

    *o << "http request methods:\n";
    for (const auto &kv : counters_) {
      *o << "  " << HttpMethodToString(kv.first) << ": " << kv.second << "\n";
    }
  }

 private:
  std::map<s3::base::HttpMethod, int> counters_;
  std::mutex mutex_;
};

HttpMethodCounters *GetHttpMethodCounters() {
  static auto *counters = new HttpMethodCounters();
  return counters;
}

void StatsWriter(std::ostream *o) {
  o->setf(std::ostream::fixed);

  *o << "http requests:\n"
        "  count: "
     << s_run_count
     << "\n"
        "  total time: "
     << std::setprecision(2) << s_run_time
     << " s\n"
        "  avg time per request: "
     << std::setprecision(3)
     << s_run_time / static_cast<double>(s_run_count) * 1.0e3
     << " ms\n"
        "  bytes: "
     << s_total_bytes
     << "\n"
        "  throughput: "
     << static_cast<double>(s_total_bytes) / s_run_time * 1.0e-3
     << " kB/s\n"
        "  curl failures: "
     << s_curl_failures
     << "\n"
        "  request failures: "
     << s_request_failures
     << "\n"
        "  timeouts: "
     << s_timeouts
     << "\n"
        "  aborts: "
     << s_aborts
     << "\n"
        "  hook retries: "
     << s_hook_retries
     << "\n"
        "  rewinds: "
     << s_rewinds << "\n";

  GetHttpMethodCounters()->Write(o);
}

Statistics::Writers::Entry s_writer(StatsWriter, 0);

RequestHook *s_request_hook = nullptr;
}  // namespace

class CurlSListWrapper {
 public:
  CurlSListWrapper() = default;
  ~CurlSListWrapper() {
    if (list_) curl_slist_free_all(list_);
  }

  inline void Append(const std::string &item) {
    list_ = curl_slist_append(list_, item.c_str());
  }

  inline const curl_slist *get() const { return list_; }

 private:
  curl_slist *list_ = nullptr;
};

class Transport {
 public:
  Transport() {
    {
      std::lock_guard<std::mutex> lock(s_mutex);
      if (s_refcount == 0) {
        curl_global_init(CURL_GLOBAL_ALL);
        auto *ver = curl_version_info(CURLVERSION_NOW);
        if (!ver) throw std::runtime_error("curl_version_info() failed.");
        S3_LOG(LOG_DEBUG, "Transport::Transport", "ssl version: %s\n",
               ver->ssl_version);
        if (!ver->ssl_version)
          throw std::runtime_error(
              "curl does not report an SSL library. cannot continue.");
#ifdef HAVE_GNUTLS
        if (strstr(ver->ssl_version, "GnuTLS")) {
          if (gnutls_global_init() != GNUTLS_E_SUCCESS)
            throw std::runtime_error("failed to initialize GnuTLS.");
        }
#endif
      }
      ++s_refcount;
    }

    curl_ = curl_easy_init();
    if (!curl_) throw std::runtime_error("curl_easy_init() failed.");
  }

  ~Transport() {
    curl_easy_cleanup(curl_);

    {
      std::lock_guard<std::mutex> lock(s_mutex);
      --s_refcount;
      if (s_refcount == 0) {
        curl_global_cleanup();
      }
    }
  }

  inline CURL *curl() const { return curl_; }

 private:
  static std::mutex s_mutex;
  static int s_refcount;

  CURL *curl_ = nullptr;
};

std::mutex Transport::s_mutex;
int Transport::s_refcount = 0;

const char *HttpMethodToString(HttpMethod method) {
  switch (method) {
    case HttpMethod::INVALID:
      return "INVALID";
    case HttpMethod::DELETE:
      return "DELETE";
    case HttpMethod::GET:
      return "GET";
    case HttpMethod::HEAD:
      return "HEAD";
    case HttpMethod::POST:
      return "POST";
    case HttpMethod::PUT:
      return "PUT";
  }
  throw std::runtime_error("invalid HTTP method.");
}

void RequestFactory::SetHook(RequestHook *hook) { s_request_hook = hook; }

std::unique_ptr<Request> RequestFactory::New() {
  return std::unique_ptr<Request>(new Request(s_request_hook));
}

std::unique_ptr<Request> RequestFactory::NewNoHook() {
  return std::unique_ptr<Request>(new Request(nullptr));
}

Request::Request(RequestHook *hook) : transport_(new Transport()), hook_(hook) {
  // stuff that's set in the ctor shouldn't be modified elsewhere, since the
  // call to init() won't reset it

  TEST_OK(curl_easy_setopt(transport_->curl(), CURLOPT_VERBOSE,
                           Config::verbose_requests()));
  TEST_OK(curl_easy_setopt(transport_->curl(), CURLOPT_NOPROGRESS, false));
  TEST_OK(curl_easy_setopt(transport_->curl(), CURLOPT_FOLLOWLOCATION, true));
  TEST_OK(curl_easy_setopt(transport_->curl(), CURLOPT_UNRESTRICTED_AUTH,
                           true));
  TEST_OK(curl_easy_setopt(transport_->curl(), CURLOPT_ERRORBUFFER,
                           transport_error_));
  static_assert(sizeof(transport_error_) >= CURL_ERROR_SIZE,
                "error buffer is too small.");
  TEST_OK(curl_easy_setopt(transport_->curl(), CURLOPT_FILETIME, true));
  TEST_OK(curl_easy_setopt(transport_->curl(), CURLOPT_NOSIGNAL, true));
  TEST_OK(curl_easy_setopt(transport_->curl(), CURLOPT_HEADERFUNCTION,
                           &Request::ProcessHeaderWrapper));
  TEST_OK(curl_easy_setopt(transport_->curl(), CURLOPT_HEADERDATA, this));
  TEST_OK(curl_easy_setopt(transport_->curl(), CURLOPT_WRITEFUNCTION,
                           &Request::WriteOutputWrapper));
  TEST_OK(curl_easy_setopt(transport_->curl(), CURLOPT_WRITEDATA, this));
  TEST_OK(curl_easy_setopt(transport_->curl(), CURLOPT_READFUNCTION,
                           &Request::ReadInputWrapper));
  TEST_OK(curl_easy_setopt(transport_->curl(), CURLOPT_READDATA, this));
  TEST_OK(curl_easy_setopt(transport_->curl(), CURLOPT_SEEKFUNCTION,
                           &Request::SeekInputWrapper));
  TEST_OK(curl_easy_setopt(transport_->curl(), CURLOPT_SEEKDATA, this));
  TEST_OK(curl_easy_setopt(transport_->curl(), CURLOPT_XFERINFOFUNCTION,
                           &Request::ProgressWrapper));
  TEST_OK(curl_easy_setopt(transport_->curl(), CURLOPT_XFERINFODATA, this));
  TEST_OK(curl_easy_setopt(transport_->curl(), CURLOPT_USERAGENT, USER_AGENT));
}

Request::~Request() {
  if (total_bytes_transferred_ > 0) {
    std::lock_guard<std::mutex> lock(s_stats_mutex);

    s_run_count += run_count_;
    s_run_time += total_run_time_;
    s_total_bytes += total_bytes_transferred_;
  }
}

void Request::Init(HttpMethod method) {
  url_.clear();
  transport_url_.clear();
  response_headers_.clear();
  output_buffer_.clear();
  response_code_ = 0;
  last_modified_ = 0;
  headers_.clear();
  input_buffer_.clear();

  TEST_OK(curl_easy_setopt(transport_->curl(), CURLOPT_CUSTOMREQUEST, nullptr));
  TEST_OK(curl_easy_setopt(transport_->curl(), CURLOPT_UPLOAD, false));
  TEST_OK(curl_easy_setopt(transport_->curl(), CURLOPT_NOBODY, false));
  TEST_OK(curl_easy_setopt(transport_->curl(), CURLOPT_POST, false));

  if (method == HttpMethod::DELETE) {
    TEST_OK(
        curl_easy_setopt(transport_->curl(), CURLOPT_CUSTOMREQUEST, "DELETE"));
    TEST_OK(curl_easy_setopt(transport_->curl(), CURLOPT_NOBODY, true));
  } else if (method == HttpMethod::HEAD) {
    TEST_OK(curl_easy_setopt(transport_->curl(), CURLOPT_NOBODY, true));
  } else if (method == HttpMethod::POST) {
    TEST_OK(curl_easy_setopt(transport_->curl(), CURLOPT_POST, true));
  } else if (method == HttpMethod::PUT) {
    TEST_OK(curl_easy_setopt(transport_->curl(), CURLOPT_UPLOAD, true));
  }

  method_ = method;
}

std::string Request::GetOutputAsString() const {
  if (output_buffer_.empty()) return "";
  // we do this because output_buffer_ has no trailing null
  std::string s;
  s.assign(&output_buffer_[0], output_buffer_.size());
  return s;
}

void Request::SetUrl(const std::string &url, const std::string &query_string) {
  url_ = url;
  transport_url_ = hook_ ? hook_->AdjustUrl(url) : url;
  if (!query_string.empty()) {
    transport_url_ +=
        ((transport_url_.find('?') == std::string::npos) ? "?" : "&");
    transport_url_ += query_string;
  }
}

void Request::SetHeader(const std::string &name, const std::string &value) {
  headers_[name] = value;
}

void Request::SetInputBuffer(std::vector<char> &&buffer) {
  input_buffer_ = std::move(buffer);
}

void Request::SetInputBuffer(const std::string &str) {
  input_buffer_.resize(str.size());
  str.copy(&input_buffer_[0], str.size());
}

void Request::ResetCurrentRunTime() { current_run_time_ = 0.0; }

void Request::Run(int timeout_in_s) {
  // sanity
  if (method_ == HttpMethod::INVALID)
    throw std::runtime_error("call Init() first!");
  if (url_.empty()) throw std::runtime_error("call SetUrl() first!");

  TEST_OK(curl_easy_setopt(transport_->curl(), CURLOPT_URL,
                           transport_url_.c_str()));

  if (method_ == HttpMethod::PUT)
    TEST_OK(curl_easy_setopt(transport_->curl(), CURLOPT_INFILESIZE_LARGE,
                             static_cast<curl_off_t>(input_buffer_.size())));
  else if (method_ == HttpMethod::POST)
    TEST_OK(curl_easy_setopt(transport_->curl(), CURLOPT_POSTFIELDSIZE_LARGE,
                             static_cast<curl_off_t>(input_buffer_.size())));
  else if (!input_buffer_.empty())
    throw std::runtime_error(
        "can't set input data for non-POST/non-PUT request.");

  CURLcode r = CURLE_OK;
  int iter = 0;
  double elapsed_time = 0.0;
  uint64_t bytes_transferred = 0;
  std::string error;

  for (; iter <= Config::max_transfer_retries(); iter++) {
    if (hook_) hook_->PreRun(this, iter);

    CurlSListWrapper headers;
    uint64_t request_size = 0;
    for (const auto &pair : headers_) {
      std::string header = pair.first + ": " + pair.second;
      headers.Append(header);
      request_size += header.size();
    }
    TEST_OK(curl_easy_setopt(transport_->curl(), CURLOPT_HTTPHEADER,
                             headers.get()));

    request_size += input_buffer_.size();

    Rewind();

    transport_error_[0] = '\0';
    output_buffer_.clear();
    response_headers_.clear();

    deadline_ = time(nullptr) + ((timeout_in_s == DEFAULT_REQUEST_TIMEOUT)
                                     ? Config::request_timeout_in_s()
                                     : timeout_in_s);

    GetHttpMethodCounters()->Increment(method_);
    r = curl_easy_perform(transport_->curl());

    switch (r) {
      case CURLE_COULDNT_RESOLVE_PROXY:
      case CURLE_COULDNT_RESOLVE_HOST:
      case CURLE_COULDNT_CONNECT:
      case CURLE_PARTIAL_FILE:
      case CURLE_UPLOAD_FAILED:
      case CURLE_OPERATION_TIMEDOUT:
      case CURLE_SSL_CONNECT_ERROR:
      case CURLE_GOT_NOTHING:
      case CURLE_SEND_ERROR:
      case CURLE_RECV_ERROR:
      case CURLE_BAD_CONTENT_ENCODING: {
        ++s_curl_failures;
        error = std::string("Recoverable error: ") + transport_error_;
        S3_LOG(LOG_WARNING, "Request::Run", "got error [%s]. retrying.\n",
               transport_error_);
        Timer::Sleep(1);
        continue;
      }

      case CURLE_ABORTED_BY_CALLBACK: {
        ++s_timeouts;
        error = "Recoverable error: timed out";
        S3_LOG(LOG_WARNING, "Request::Run", "timed out for [%s]. retrying.\n",
               url_.c_str());
        Timer::Sleep(1);
        continue;
      }

      default:
        // Do nothing.
        error = std::string("Unrecoverable error (") + curl_easy_strerror(r) +
            "): " + transport_error_;
        break;
    }

    if (r == CURLE_OK) {
      double this_iter_et = 0.0;

      TEST_OK(curl_easy_getinfo(transport_->curl(), CURLINFO_RESPONSE_CODE,
                                &response_code_));
      TEST_OK(curl_easy_getinfo(transport_->curl(), CURLINFO_TOTAL_TIME,
                                &this_iter_et));
      TEST_OK(curl_easy_getinfo(transport_->curl(), CURLINFO_FILETIME,
                                &last_modified_));

      elapsed_time += this_iter_et;
      bytes_transferred += request_size + output_buffer_.size();

      if (hook_ && hook_->ShouldRetry(this, iter)) {
        ++s_hook_retries;
        continue;
      }
    }

    // break on CURLE_OK or some other error where we don't want to try the
    // request again
    break;
  }

  if (r != CURLE_OK) {
    ++s_aborts;
    throw std::runtime_error(error);
  }

  // don't save the time for the first request since it's likely to be
  // disproportionately large
  if (run_count_ > 0) {
    total_run_time_ += elapsed_time;
    total_bytes_transferred_ += bytes_transferred;
  }

  // but save it in current_run_time_ since it's compared to overall function
  // time (i.e., it's relative)
  current_run_time_ += elapsed_time;
  run_count_ += iter + 1;

  if (response_code_ >= HTTP_SC_BAD_REQUEST &&
      response_code_ != HTTP_SC_NOT_FOUND) {
    ++s_request_failures;
    S3_LOG(LOG_WARNING, "Request::Run",
           "request for [%s] [%s] failed with code %i and response: %s\n",
           HttpMethodToString(method_), url_.c_str(), response_code_,
           GetOutputAsString().c_str());
  }
}

size_t Request::ProcessHeader(char *data, size_t size, size_t items) {
  size *= items;
  if (data[size] != '\0')
    return size;  // we choose not to handle the case where data isn't
                  // null-terminated

  const char *p1 = strchr(data, ':');
  if (!p1) return size;  // no colon means it's not a header we care about

  std::string name(data, p1 - data);
  if (*++p1 == ' ') p1++;

  // for some reason the ETag header (among others?) contains a carriage return
  const char *p2 = strpbrk(p1, "\r\n");
  if (!p2) p2 = p1 + strlen(p1);

  std::string value(p1, p2 - p1);
  response_headers_[name] = value;

  return size;
}

size_t Request::WriteOutput(char *data, size_t size, size_t items) {
  // why even bother with "items"?
  size *= items;

  size_t old_size = output_buffer_.size();
  output_buffer_.resize(old_size + size);
  memcpy(&output_buffer_[old_size], data, size);

  return size;
}

size_t Request::ReadInput(char *data, size_t size, size_t items) {
  size *= items;

  size_t remaining = std::min(input_remaining_, size);
  memcpy(data, input_pos_, remaining);
  input_pos_ += remaining;
  input_remaining_ -= remaining;

  return remaining;
}

int Request::SeekInput(off_t offset, int origin) {
  S3_LOG(LOG_DEBUG, "Request::SeekInput", "seek to [%jd] from [%i] for [%s]\n",
         static_cast<intmax_t>(offset), origin, url_.c_str());

  if (origin != SEEK_SET || offset != 0) return CURL_SEEKFUNC_FAIL;

  Rewind();
  ++s_rewinds;
  return CURL_SEEKFUNC_OK;
}

int Request::Progress(off_t dl_total, off_t dl_now, off_t ul_total,
                      off_t ul_now) {
  if (time(nullptr) > deadline_) {
    S3_LOG(LOG_DEBUG, "Request::Progress", "time out for [%s]\n", url_.c_str());
    return 1;
  }

  return 0;
}

void Request::Rewind() {
  input_pos_ = input_buffer_.empty() ? nullptr : &input_buffer_[0];
  input_remaining_ = input_buffer_.size();
}

}  // namespace base
}  // namespace s3
