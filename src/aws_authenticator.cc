#include <errno.h>

#include <stdexcept>

#include "aws_authenticator.h"
#include "config.h"
#include "logger.h"
#include "request.h"
#include "util.h"

using namespace std;

using namespace s3;

namespace
{
  const string AMZ_HEADER_PREFIX = "x-amz-";
  const string EMPTY = "";

  const string & safe_find(const header_map &map, const char *key)
  {
    header_map::const_iterator itor = map.find(key);

    return (itor == map.end()) ? EMPTY : itor->second;
  }
}

void aws_authenticator::sign(request *req)
{
  const header_map &headers = req->get_headers();
  string sig = "AWS " + config::get_aws_key() + ":";
  string to_sign = 
    req->get_method() + "\n" + 
    safe_find(headers, "Content-MD5") + "\n" + 
    safe_find(headers, "Content-Type") + "\n" + 
    safe_find(headers, "Date") + "\n";

  for (header_map::const_iterator itor = headers.begin(); itor != headers.end(); ++itor)
    if (!itor->second.empty() && itor->first.substr(0, AMZ_HEADER_PREFIX.size()) == AMZ_HEADER_PREFIX)
      to_sign += itor->first + ":" + itor->second + "\n";

  to_sign += req->get_url();
  req->set_header("Authorization", string("AWS ") + config::get_aws_key() + ":" + util::sign(config::get_aws_secret(), to_sign));
}
