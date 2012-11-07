#include "base/logger.h"
#include "base/request.h"
#include "objects/symlink.h"

using boost::defer_lock;
using boost::mutex;
using std::string;

using s3::base::request;
using s3::objects::object;
using s3::objects::symlink;

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

    return new s3::objects::symlink(path);
  }

  object::type_checker_list::entry s_checker_reg(checker, 100);
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

int symlink::internal_read(const request::ptr &req)
{
  mutex::scoped_lock lock(_mutex, defer_lock);
  string output;

  req->init(base::HTTP_GET);
  req->set_url(get_url());

  req->run();
  
  if (req->get_response_code() != base::HTTP_SC_OK)
    return -EIO;

  output = req->get_output_string();

  if (strncmp(output.c_str(), CONTENT_PREFIX_CSTR, CONTENT_PREFIX_LEN) != 0) {
    S3_LOG(LOG_WARNING, "symlink::internal_read", "content prefix does not match: [%s]\n", output.c_str());
    return -EINVAL;
  }

  lock.lock();
  _target = output.c_str() + CONTENT_PREFIX_LEN;

  return 0;
}
