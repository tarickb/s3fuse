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

#include <stdint.h>
#include <string.h>

#include <stdexcept>
#include <boost/detail/atomic_count.hpp>

#include "config.h"
#include "logger.h"
#include "request.h"
#include "request_hook.h"
#include "ssl_locks.h"
#include "statistics.h"
#include "timer.h"

using boost::mutex;
using boost::detail::atomic_count;
using std::min;
using std::ostream;
using std::runtime_error;
using std::setprecision;
using std::string;

using s3::base::request;
using s3::base::statistics;
using s3::base::timer;

#define TEST_OK(x) do { if ((x) != CURLE_OK) throw runtime_error("call to " #x " failed."); } while (0)

namespace
{
  class curl_slist_wrapper
  {
  public:
    curl_slist_wrapper()
      : _list(NULL)
    {
    }

    ~curl_slist_wrapper()
    {
      if (_list)
        curl_slist_free_all(_list);
    }

    inline void append(const char *item)
    {
      _list = curl_slist_append(_list, item);
    }

    inline const curl_slist * get() const
    {
      return _list;
    }

  private:
    curl_slist *_list;
  };

  uint64_t s_run_count = 0;
  uint64_t s_total_bytes = 0;
  double s_run_time = 0.0;
  atomic_count s_curl_failures(0), s_request_failures(0);
  atomic_count s_timeouts(0), s_aborts(0), s_hook_retries(0);
  atomic_count s_rewinds(0);

  mutex s_stats_mutex;

  void statistics_writer(ostream *o)
  {
    o->setf(ostream::fixed);

    *o << 
      "http requests:\n"
      "  count: " << s_run_count << "\n"
      "  total time: " << setprecision(2) << s_run_time << " s\n"
      "  avg time per request: " << setprecision(3) << s_run_time / static_cast<double>(s_run_count) * 1.0e3 << " ms\n"
      "  bytes: " << s_total_bytes << "\n"
      "  throughput: " << static_cast<double>(s_total_bytes) / s_run_time * 1.0e-3 << " kB/s\n"
      "  curl failures: " << s_curl_failures << "\n"
      "  request failures: " << s_request_failures << "\n"
      "  timeouts: " << s_timeouts << "\n"
      "  aborts: " << s_aborts << "\n"
      "  hook retries: " << s_hook_retries << "\n"
      "  rewinds: " << s_rewinds << "\n";
  }

  statistics::writers::entry s_writer(statistics_writer, 0);
}

size_t request::header_process(char *data, size_t size, size_t items, void *context)
{
  request *req = static_cast<request *>(context);
  char *pos;

  size *= items;

  if (req->_canceled)
    return 0; // abort!

  if (data[size] != '\0')
    return size; // we choose not to handle the case where data isn't null-terminated

  pos = strchr(data, '\n');

  if (pos)
    *pos = '\0';

  // for some reason the ETag header (among others?) contains a carriage return
  pos = strchr(data, '\r'); 

  if (pos)
    *pos = '\0';

  pos = strchr(data, ':');

  if (!pos)
    return size; // no colon means it's not a header we care about

  *pos++ = '\0';

  if (*pos == ' ')
    pos++;

  req->_response_headers[data] = pos;

  return size;
}

size_t request::output_write(char *data, size_t size, size_t items, void *context)
{
  request *req = static_cast<request *>(context);
  size_t old_size;

  // why even bother with "items"?
  size *= items;

  if (req->_canceled)
    return 0; // abort!

  old_size = req->_output_buffer.size();
  req->_output_buffer.resize(old_size + size);
  memcpy(&req->_output_buffer[old_size], data, size);

  return size;
}

size_t request::input_read(char *data, size_t size, size_t items, void *context)
{
  size_t remaining;
  request *req = static_cast<request *>(context);

  size *= items;

  if (req->_canceled)
    return 0; // abort!

  remaining = min(req->_input_remaining, size);

  memcpy(data, req->_input_pos, remaining);
  
  req->_input_pos += remaining;
  req->_input_remaining -= remaining;

  return remaining;
}

int request::input_seek(void *context, curl_off_t offset, int origin)
{
  request *req = static_cast<request *>(context);

  S3_LOG(LOG_DEBUG, "request::input_seek", "seek to [%" CURL_FORMAT_CURL_OFF_T "] from [%i] for [%s]\n", offset, origin, req->_url.c_str());

  if (origin != SEEK_SET || offset != 0)
    return CURL_SEEKFUNC_FAIL;

  req->rewind();
  ++s_rewinds;

  return CURL_SEEKFUNC_OK;
}

request::request()
  : _hook(NULL),
    _current_run_time(0.0),
    _total_run_time(0.0),
    _run_count(0),
    _total_bytes_transferred(0),
    _canceled(false),
    _timeout(0)
{
  _curl = curl_easy_init();

  if (!_curl)
    throw runtime_error("curl_easy_init() failed.");

  ssl_locks::init();

  // stuff that's set in the ctor shouldn't be modified elsewhere, since the call to init() won't reset it

  TEST_OK(curl_easy_setopt(_curl, CURLOPT_VERBOSE, config::get_verbose_requests()));
  TEST_OK(curl_easy_setopt(_curl, CURLOPT_NOPROGRESS, true));
  TEST_OK(curl_easy_setopt(_curl, CURLOPT_FOLLOWLOCATION, true));
  TEST_OK(curl_easy_setopt(_curl, CURLOPT_ERRORBUFFER, _curl_error));
  TEST_OK(curl_easy_setopt(_curl, CURLOPT_FILETIME, true));
  TEST_OK(curl_easy_setopt(_curl, CURLOPT_NOSIGNAL, true));
  TEST_OK(curl_easy_setopt(_curl, CURLOPT_HEADERFUNCTION, &request::header_process));
  TEST_OK(curl_easy_setopt(_curl, CURLOPT_HEADERDATA, this));
  TEST_OK(curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, &request::output_write));
  TEST_OK(curl_easy_setopt(_curl, CURLOPT_WRITEDATA, this));
  TEST_OK(curl_easy_setopt(_curl, CURLOPT_READFUNCTION, &request::input_read));
  TEST_OK(curl_easy_setopt(_curl, CURLOPT_READDATA, this));
  TEST_OK(curl_easy_setopt(_curl, CURLOPT_SEEKFUNCTION, &request::input_seek));
  TEST_OK(curl_easy_setopt(_curl, CURLOPT_SEEKDATA, this));
}

request::~request()
{
  curl_easy_cleanup(_curl);

  if (_total_bytes_transferred > 0) {
    mutex::scoped_lock lock(s_stats_mutex);

    s_run_count += _run_count;
    s_run_time += _total_run_time;
    s_total_bytes += _total_bytes_transferred;
  }

  ssl_locks::release();
}

void request::init(http_method method)
{
  if (_canceled)
    throw runtime_error("cannot reuse a canceled request.");

  _curl_error[0] = '\0';
  _url.clear();
  _curl_url.clear();
  _output_buffer.clear();
  _response_headers.clear();
  _response_code = 0;
  _last_modified = 0;
  _headers.clear();
  _input_buffer.reset();

  TEST_OK(curl_easy_setopt(_curl, CURLOPT_CUSTOMREQUEST, NULL));
  TEST_OK(curl_easy_setopt(_curl, CURLOPT_UPLOAD, false));
  TEST_OK(curl_easy_setopt(_curl, CURLOPT_NOBODY, false));
  TEST_OK(curl_easy_setopt(_curl, CURLOPT_POST, false));

  if (method == HTTP_DELETE) {
    _method = "DELETE";
    TEST_OK(curl_easy_setopt(_curl, CURLOPT_CUSTOMREQUEST, "DELETE"));
    TEST_OK(curl_easy_setopt(_curl, CURLOPT_NOBODY, true));

  } else if (method == HTTP_GET) {
    _method = "GET";

  } else if (method == HTTP_HEAD) {
    _method = "HEAD";
    TEST_OK(curl_easy_setopt(_curl, CURLOPT_NOBODY, true));

  } else if (method == HTTP_POST) {
    _method = "POST";
    TEST_OK(curl_easy_setopt(_curl, CURLOPT_POST, true));

  } else if (method == HTTP_PUT) {
    _method = "PUT";
    TEST_OK(curl_easy_setopt(_curl, CURLOPT_UPLOAD, true));

  } else
    throw runtime_error("unsupported HTTP method.");
}

void request::set_url(const string &url, const string &query_string)
{
  _url = url;
  _curl_url = _hook ? _hook->adjust_url(url) : url;

  if (!query_string.empty()) {
    _curl_url += ((_curl_url.find('?') == string::npos) ? "?" : "&");
    _curl_url += query_string;
  }
}

bool request::check_timeout()
{
  if (_timeout && time(NULL) > _timeout) {
    S3_LOG(LOG_WARNING, "request::check_timeout", "timed out on [%s] [%s].\n", _method.c_str(), _url.c_str());

    _canceled = true;
    return true;
  }

  return false;
}

void request::run(int timeout_in_s)
{
  int r = CURLE_OK;
  int iter;
  double elapsed_time = 0.0;
  uint64_t bytes_transferred = 0;

  // sanity
  if (_url.empty())
    throw runtime_error("call set_url() first!");

  if (_method.empty())
    throw runtime_error("call set_method() first!");

  if (_canceled)
    throw runtime_error("cannot reuse a canceled request.");

  TEST_OK(curl_easy_setopt(_curl, CURLOPT_URL, _curl_url.c_str()));

  if (_method == "PUT")
    TEST_OK(curl_easy_setopt(_curl, CURLOPT_INFILESIZE_LARGE, _input_buffer ? static_cast<curl_off_t>(_input_buffer->size()) : 0));
  else if (_method == "POST")
    TEST_OK(curl_easy_setopt(_curl, CURLOPT_POSTFIELDSIZE_LARGE, _input_buffer ? static_cast<curl_off_t>(_input_buffer->size()) : 0));
  else if (_input_buffer && !_input_buffer->empty())
    throw runtime_error("can't set input data for non-POST/non-PUT request.");

  for (iter = 0; iter < config::get_max_transfer_retries(); iter++) {
    curl_slist_wrapper headers;
    uint64_t request_size = 0;
   
    _output_buffer.clear();
    _response_headers.clear();

    if (_hook)
      _hook->pre_run(this, iter);

    for (header_map::const_iterator itor = _headers.begin(); itor != _headers.end(); ++itor) {
      string header = itor->first + ": " + itor->second;

      headers.append(header.c_str());
      request_size += header.size();
    }

    TEST_OK(curl_easy_setopt(_curl, CURLOPT_HTTPHEADER, headers.get()));

    if (_input_buffer)
      request_size += _input_buffer->size();

    rewind();

    _timeout = time(NULL) + ((timeout_in_s == DEFAULT_REQUEST_TIMEOUT) ? config::get_request_timeout_in_s() : timeout_in_s);
    r = curl_easy_perform(_curl);
    _timeout = 0; // reset this here so that subsequent calls to check_timeout() don't fail

    if (_canceled) {
      ++s_timeouts;
      throw runtime_error("request timed out.");
    }

    if (
      r == CURLE_COULDNT_RESOLVE_PROXY || 
      r == CURLE_COULDNT_RESOLVE_HOST || 
      r == CURLE_COULDNT_CONNECT || 
      r == CURLE_PARTIAL_FILE || 
      r == CURLE_UPLOAD_FAILED || 
      r == CURLE_OPERATION_TIMEDOUT || 
      r == CURLE_SSL_CONNECT_ERROR || 
      r == CURLE_GOT_NOTHING || 
      r == CURLE_SEND_ERROR || 
      r == CURLE_RECV_ERROR || 
      r == CURLE_BAD_CONTENT_ENCODING)
    {
      ++s_curl_failures;
      S3_LOG(LOG_WARNING, "request::run", "got error [%s]. retrying.\n", _curl_error);
      timer::sleep(1);

      continue;
    }

    if (r == CURLE_OK) {
      double this_iter_et = 0.0;

      TEST_OK(curl_easy_getinfo(_curl, CURLINFO_RESPONSE_CODE, &_response_code));
      TEST_OK(curl_easy_getinfo(_curl, CURLINFO_TOTAL_TIME, &this_iter_et));
      TEST_OK(curl_easy_getinfo(_curl, CURLINFO_FILETIME, &_last_modified));

      elapsed_time += this_iter_et;
      bytes_transferred += request_size + _output_buffer.size();

      if (_hook && _hook->should_retry(this, iter)) {
        ++s_hook_retries;
        continue;
      }
    }

    // break on CURLE_OK or some other error where we don't want to try the request again
    break;
  }

  if (r != CURLE_OK) {
    ++s_aborts;
    throw runtime_error(_curl_error);
  }

  // don't save the time for the first request since it's likely to be disproportionately large
  if (_run_count > 0) {
    _total_run_time += elapsed_time;
    _total_bytes_transferred += bytes_transferred;
  }

  // but save it in _current_run_time since it's compared to overall function time (i.e., it's relative)
  _current_run_time += elapsed_time;

  _run_count += iter + 1;

  if (_response_code >= HTTP_SC_MULTIPLE_CHOICES && _response_code != HTTP_SC_NOT_FOUND) {
    ++s_request_failures;

    S3_LOG(
      LOG_WARNING, 
      "request::run", 
      "request for [%s] [%s] failed with code %i and response: %s\n", 
      _method.c_str(),
      _url.c_str(), 
      _response_code, 
      get_output_string().c_str());
  }
}

