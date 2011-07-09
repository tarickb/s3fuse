#include <fstream>
#include <vector>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include "aws_authenticator.h"
#include "config.h"
#include "logger.h"
#include "request.h"
#include "util.h"

using namespace boost;
using namespace std;

using namespace s3;

namespace
{
  const string AWS_URL_PREFIX = "https://s3.amazonaws.com";
  const string AWS_XML_NAMESPACE = "http://s3.amazonaws.com/doc/2006-03-01/";
  const string AMZ_HEADER_PREFIX = "x-amz-";
  const string EMPTY = "";

  const string & safe_find(const header_map &map, const char *key)
  {
    header_map::const_iterator itor = map.find(key);

    return (itor == map.end()) ? EMPTY : itor->second;
  }
}

aws_authenticator::aws_authenticator()
{
  vector<string> fields;

  split(fields, config::get_auth_data(), is_any_of(" \t"), token_compress_on);

  if (fields.size() != 2) {
    S3_LOG(
      LOG_CRIT, 
      "aws_authenticator::aws_authenticator", 
      "expected 2 fields for auth_data, found %i.\n",
      fields.size());

    throw runtime_error("error while parsing auth data for AWS.");
  }

  _key = fields[0];
  _secret = fields[1];
}

const string & aws_authenticator::get_url_prefix()
{
  return AWS_URL_PREFIX;
}

const string & aws_authenticator::get_xml_namespace()
{
  return AWS_XML_NAMESPACE;
}

void aws_authenticator::sign(request *req)
{
  const header_map &headers = req->get_headers();
  string to_sign = 
    req->get_method() + "\n" + 
    safe_find(headers, "Content-MD5") + "\n" + 
    safe_find(headers, "Content-Type") + "\n" + 
    safe_find(headers, "Date") + "\n";

  for (header_map::const_iterator itor = headers.begin(); itor != headers.end(); ++itor)
    if (!itor->second.empty() && itor->first.substr(0, AMZ_HEADER_PREFIX.size()) == AMZ_HEADER_PREFIX)
      to_sign += itor->first + ":" + itor->second + "\n";

  to_sign += req->get_url();
  req->set_header("Authorization", string("AWS ") + _key + ":" + util::sign(_secret, to_sign));
}
