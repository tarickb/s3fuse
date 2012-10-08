#include "logger.h"
#include "request.h"
#include "symlink.h"

using namespace boost;
using namespace std;

using namespace s3;

namespace
{
  const string CONTENT_TYPE = "text/symlink";

  const string CONTENT_PREFIX = "SYMLINK:";
  const char * CONTENT_PREFIX_CSTR = CONTENT_PREFIX.c_str();
  const size_t CONTENT_PREFIX_LEN = CONTENT_PREFIX.size();

  object * checker(const string &path, const request::ptr &req)
  {
    if (req->get_response_header("Content-Type") != CONTENT_TYPE)
      return NULL;

    return new s3::symlink(path);
  }

  object::type_checker::type_checker s_checker_reg(checker, 100);
}

symlink::symlink(const string &path)
  : object(path)
{
  set_content_type(CONTENT_TYPE);
  set_object_type(S_IFLNK);
}

symlink::~symlink()
{
}

void symlink::set_request_body(const request::ptr &req)
{
  mutex::scoped_lock lock(_mutex);

  req->set_input_buffer(CONTENT_PREFIX + _target);
}

int symlink::read(const request::ptr &req, string *target)
{
  req->init(HTTP_GET);
  req->set_url(get_url());

  req->run();
  
  if (req->get_response_code() != HTTP_SC_OK)
    return -EIO;

  if (strncmp(req->get_output_buffer(), CONTENT_PREFIX_CSTR, CONTENT_PREFIX_LEN) != 0) {
    S3_LOG(LOG_WARNING, "symlink::read", "content prefix does not match: [%s]\n", req->get_output_buffer());
    return -EINVAL;
  }

  *target = req->get_output_buffer() + CONTENT_PREFIX_LEN;
  return 0;
}
