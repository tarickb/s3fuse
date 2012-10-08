/*
 * request.h
 * -------------------------------------------------------------------------
 * Mostly a wrapper around a libcurl handle.
 * -------------------------------------------------------------------------
 *
 * Copyright (c) 2011, Tarick Bedeir.
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

#ifndef S3_REQUEST_H
#define S3_REQUEST_H

#include <stdint.h>
#include <stdio.h>
#include <curl/curl.h>

#include <map>
#include <string>
#include <vector>
#include <boost/smart_ptr.hpp>
#include <boost/utility.hpp>

namespace s3
{
  class object;
  class worker_thread;

  enum http_method
  {
    HTTP_DELETE,
    HTTP_GET,
    HTTP_HEAD,
    HTTP_POST,
    HTTP_PUT
  };

  enum http_status_code
  {
    HTTP_SC_OK = 200,
    HTTP_SC_NO_CONTENT = 204,
    HTTP_SC_PARTIAL_CONTENT = 206,
    HTTP_SC_MULTIPLE_CHOICES = 300,
    HTTP_SC_UNAUTHORIZED = 401,
    HTTP_SC_FORBIDDEN = 403,
    HTTP_SC_NOT_FOUND = 404,
    HTTP_SC_INTERNAL_SERVER_ERROR = 500,
    HTTP_SC_SERVICE_UNAVAILABLE = 503
  };

  typedef std::map<std::string, std::string> header_map;

  class request : boost::noncopyable
  {
  public:
    static const int DEFAULT_REQUEST_TIMEOUT = -1;

    typedef boost::shared_ptr<request> ptr;

    request();
    ~request();

    void init(http_method method);

    inline const std::string & get_method() { return _method; }

    void set_full_url(const std::string &url);
    void set_url(const std::string &url, const std::string &query_string = "");
    inline const std::string & get_url() { return _url; }

    inline const header_map & get_headers() { return _headers; }
    inline void set_header(const std::string &name, const std::string &value) { _headers[name] = value; }

    void set_input_buffer(const char *buffer, size_t size);

    inline void set_input_buffer(const std::string &buffer)
    { 
      // make a copy so that we're not holding a pointer to a parameter that
      // was passed to us as a const reference -- that would not be cool

      _input_string_copy = buffer;

      set_input_buffer(_input_string_copy.c_str(), _input_string_copy.size());
    }

    inline const char * get_output_buffer() { return &_output_buffer[0]; }
    inline size_t get_output_buffer_size() { return _output_buffer.size(); }

    inline const std::string & get_response_header(const std::string &key) { return _response_headers[key]; }
    inline const header_map & get_response_headers() { return _response_headers; }

    inline long get_response_code() { return _response_code; }
    inline time_t get_last_modified() { return _last_modified; }

    inline void reset_current_run_time() { _current_run_time = 0.0; }
    inline double get_current_run_time() { return _current_run_time; }

    inline void disable_signing() { _sign = false; }

    bool check_timeout();

    void run(int timeout_in_s = DEFAULT_REQUEST_TIMEOUT);

  private:
    static size_t process_header(char *data, size_t size, size_t items, void *context);
    static size_t process_output(char *data, size_t size, size_t items, void *context);
    static size_t process_input(char *data, size_t size, size_t items, void *context);

    void reset();
    void build_request_time();
    void build_signature();

    void internal_run(int timeout_in_s);

    // review init() when making changes here
    CURL *_curl;
    char _curl_error[CURL_ERROR_SIZE];

    std::string _method;
    std::string _url;
    header_map _response_headers;

    std::vector<char> _output_buffer;

    const char *_input_buffer;
    size_t _input_size;
    std::string _input_string_copy;

    long _response_code;
    time_t _last_modified;

    header_map _headers; // assumptions: no duplicates, all header names are always lower-case

    double _current_run_time, _total_run_time;
    uint64_t _run_count;

    bool _canceled;
    time_t _timeout;

    bool _sign;
  };
}

#endif
