#ifndef S3_REQUEST_HH
#define S3_REQUEST_HH

#include <stdio.h>
#include <curl/curl.h>

#include <map>
#include <string>

namespace s3
{
  enum http_method
  {
    HTTP_GET,
    HTTP_HEAD,
    HTTP_PUT
  };

  class request
  {
  public:
    typedef std::map<std::string, std::string> header_map;

    request(http_method method);
    ~request();

    void set_url(const std::string &url, const std::string &query_string);
    inline void set_header(const std::string &name, const std::string &value) { _headers[name] = value; }

    void set_output_file(FILE *f);
    void set_input_file(FILE *f, size_t size);

    inline const std::string & get_response_data() { return _response_data; }
    inline const std::string & get_response_header(const std::string &key) { return _response_headers[key]; }
    inline long get_response_code() { return _response_code; }
    inline long get_last_modified() { return _last_modified; }

    void run();

  private:
    static size_t add_header_to_map(char *data, size_t size, size_t items, void *context);

    void build_request_time();
    void build_signature();

    CURL *_curl;
    char _curl_error[CURL_ERROR_SIZE];
    std::string _method;
    std::string _aws_key;
    std::string _aws_secret;
    std::string _url;
    std::string _response_data;
    header_map _response_headers;
    long _response_code;
    long _last_modified;
    header_map _headers; // assumptions: no duplicates, all header names are always lower-case
  };
}

#endif
