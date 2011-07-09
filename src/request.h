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

  typedef std::map<std::string, std::string> header_map;
  typedef boost::shared_ptr<header_map> header_map_ptr;

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

    void set_output_fd(int fd = -1, off_t offset = 0);
    void set_input_fd(int fd = -1, size_t size = 0, off_t offset = 0);
    void set_input_data(const std::string &s);

    inline const std::string & get_response_data() { return _output_data; }

    inline const std::string & get_response_header(const std::string &key) { return _response_headers[key]; }
    inline const header_map & get_response_headers() { return _response_headers; }

    inline long get_response_code() { return _response_code; }
    inline time_t get_last_modified() { return _last_modified; }

    inline void set_target_object(const boost::shared_ptr<object> &object) { _target_object = object; }
    inline const boost::shared_ptr<object> & get_target_object() { return _target_object; }

    inline void reset_current_run_time() { _current_run_time = 0.0; }
    inline double get_current_run_time() { return _current_run_time; }

    void set_meta_headers(const boost::shared_ptr<object> &object);

    bool check_timeout();

    void run(int timeout_in_s = DEFAULT_REQUEST_TIMEOUT);

  private:
    static size_t process_header(char *data, size_t size, size_t items, void *context);
    static size_t process_output(char *data, size_t size, size_t items, void *context);
    static size_t process_input(char *data, size_t size, size_t items, void *context);

    void reset();
    void build_request_time();
    void build_signature();

    // review reset() when making changes here
    CURL *_curl;
    char _curl_error[CURL_ERROR_SIZE];

    std::string _method;
    std::string _url;
    header_map _response_headers;

    int _output_fd;
    off_t _output_offset;
    std::string _output_data;

    int _input_fd;
    size_t _input_size;
    off_t _input_offset;
    std::string _input_data;

    boost::shared_ptr<object> _target_object;

    long _response_code;
    time_t _last_modified;

    header_map _headers; // assumptions: no duplicates, all header names are always lower-case

    double _current_run_time, _total_run_time;
    uint64_t _run_count;

    bool _canceled;
    time_t _timeout;
  };
}

#endif
