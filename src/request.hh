#ifndef S3_REQUEST_HH
#define S3_REQUEST_HH

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
    typedef boost::shared_ptr<request> ptr;

    request();
    ~request();

    void init(http_method method);

    void set_url(const std::string &url, const std::string &query_string = "");
    inline const std::string & get_url() { return _url; }

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

    void set_meta_headers(const boost::shared_ptr<object> &object);

    void run();

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

    double _total_run_time;
    uint64_t _run_count;
  };
}

#endif
