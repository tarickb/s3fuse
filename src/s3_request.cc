#include <stdint.h>
#include <string.h>
#include <time.h>
#include <pugixml/pugixml.hpp>
#include <sys/stat.h>

#include <iostream>
#include <stdexcept>
#include <boost/lexical_cast.hpp>

#include "s3_request.hh"
#include "s3_util.hh"

using namespace boost;
using namespace pugi;
using namespace std;

using namespace s3;

namespace
{
  string g_url_prefix = "https://s3.amazonaws.com";
  string g_aws_key = "AKIAJZHNXBKNRCUMV4IQ";
  string g_aws_secret = "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";
  string g_amz_header_prefix = "x-amz-";

  size_t append_to_string(char *data, size_t size, size_t items, void *context)
  {
    static_cast<string *>(context)->append(data, size * items);

    return size * items;
  }

  size_t null_readdata(void *, size_t, size_t, void *)
  {
    return 0;
  }
}

s3_request::s3_request(http_method method)
  : _aws_key(g_aws_key),
    _aws_secret(g_aws_secret),
    _response_code(0),
    _last_modified(0)
{
  _curl = curl_easy_init();

  if (!_curl)
    throw runtime_error("curl_easy_init() failed.");

  try {
    // TODO: make optional
    // curl_easy_setopt(_curl, CURLOPT_VERBOSE, 1);
    curl_easy_setopt(_curl, CURLOPT_NOPROGRESS, 1);
    curl_easy_setopt(_curl, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(_curl, CURLOPT_HEADERFUNCTION, &s3_request::add_header_to_map);
    curl_easy_setopt(_curl, CURLOPT_HEADERDATA, &_response_headers);
    curl_easy_setopt(_curl, CURLOPT_ERRORBUFFER, _curl_error);
    curl_easy_setopt(_curl, CURLOPT_FILETIME, 1);

    set_input_file(NULL, 0);
    set_output_file(NULL);

    if (method == HTTP_GET) {
      _method = "GET";

    } else if (method == HTTP_HEAD) {
      _method = "HEAD";
      curl_easy_setopt(_curl, CURLOPT_NOBODY, 1);

    } else if (method == HTTP_PUT) {
      _method = "PUT";
      curl_easy_setopt(_curl, CURLOPT_UPLOAD, 1);

    } else
      throw runtime_error("unsupported HTTP method.");

  } catch (...) {
    curl_easy_cleanup(_curl);
    throw;
  }
}

s3_request::~s3_request()
{
  curl_easy_cleanup(_curl);
}

size_t s3_request::add_header_to_map(char *data, size_t size, size_t items, void *context)
{
  header_map *headers = static_cast<header_map *>(context);
  size_t full_size = size * items;
  char *pos;

  if (data[full_size] != '\0')
    return size * items; // we choose not to handle the case where data isn't null-terminated

  pos = strchr(data, '\n');

  if (pos)
    *pos = '\0';

  pos = strchr(data, ':');

  if (!pos)
    return size * items; // no colon means it's not a header we care about

  *pos++ = '\0';

  if (*pos == ' ')
    pos++;

  headers->operator [](data) = pos;
  return size * items;
}

void s3_request::set_url(const string &url, const string &query_string)
{
  string curl_url = g_url_prefix + url;

  if (!query_string.empty()) {
    curl_url += (curl_url.find('?') == string::npos) ? "?" : "&";
    curl_url += query_string;
  }

  _url = url;
  curl_easy_setopt(_curl, CURLOPT_URL, curl_url.c_str());

  cerr << "s3_request: url: " << curl_url << endl;
}

void s3_request::set_output_file(FILE *f)
{
  if (f) {
    curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, NULL);
    curl_easy_setopt(_curl, CURLOPT_WRITEDATA, f);
  } else {
    curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, append_to_string);
    curl_easy_setopt(_curl, CURLOPT_WRITEDATA, &_response_data);
  }
}

void s3_request::set_input_file(FILE *f, size_t size)
{
  if (f) {
    curl_easy_setopt(_curl, CURLOPT_READFUNCTION, NULL);
    curl_easy_setopt(_curl, CURLOPT_READDATA, f);
    curl_easy_setopt(_curl, CURLOPT_INFILESIZE_LARGE, static_cast<curl_off_t>(size));
  } else {
    curl_easy_setopt(_curl, CURLOPT_READFUNCTION, null_readdata);
    curl_easy_setopt(_curl, CURLOPT_READDATA, NULL);
    curl_easy_setopt(_curl, CURLOPT_INFILESIZE_LARGE, static_cast<curl_off_t>(0));
  }
}

void s3_request::build_request_time()
{
  time_t sys_time;
  tm gm_time;
  char time_str[128];

  time(&sys_time);
  gmtime_r(&sys_time, &gm_time);
  strftime(time_str, 128, "%a, %d %b %Y %H:%M:%S GMT", &gm_time);

  _headers["Date"] = time_str;
}

void s3_request::build_signature()
{
  string sig = "AWS " + _aws_key + ":";
  string to_sign = _method + "\n" + _headers["Content-MD5"] + "\n" + _headers["Content-Type"] + "\n" + _headers["Date"] + "\n";

  for (header_map::const_iterator itor = _headers.begin(); itor != _headers.end(); ++itor)
    if (!itor->second.empty() && itor->first.substr(0, 6) == g_amz_header_prefix)
      to_sign += itor->first + ":" + itor->second + "\n";

  to_sign += _url;
  _headers["Authorization"] = string("AWS ") + _aws_key + ":" + util::sign(_aws_secret, to_sign);
}

void s3_request::run()
{
  curl_slist *headers = NULL;

  _response_data.clear();
  _response_headers.clear();

  build_request_time();
  build_signature();

  for (header_map::const_iterator itor = _headers.begin(); itor != _headers.end(); ++itor)
    if (!itor->second.empty())
      headers = curl_slist_append(headers, (itor->first + ": " + itor->second).c_str());

  curl_easy_setopt(_curl, CURLOPT_HTTPHEADER, headers);

  if (curl_easy_perform(_curl) != 0)
    throw runtime_error(_curl_error);

  curl_easy_getinfo(_curl, CURLINFO_RESPONSE_CODE, &_response_code);
  curl_easy_getinfo(_curl, CURLINFO_FILETIME, &_last_modified);

  // TODO: remove cerr
  // TODO: add loop for timeouts and whatnot

  if (_response_code != 200)
    cerr << "s3_request: response_code: " << _response_code << endl;
}

