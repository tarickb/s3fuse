#include "logger.h"
#include "request.h"
#include "symlink.h"

using namespace boost;
using namespace std;

using namespace s3;

namespace
{
  const string CONTENT_TYPE = "text/symlink";

  object * checker(const string &path, const request::ptr &req)
  {
    S3_LOG(LOG_DEBUG, "symlink::checker", "testing [%s]\n", path.c_str());

    if (req->get_response_header("Content-Type") != CONTENT_TYPE)
      return NULL;

    return new s3::symlink(path);
  }

  object::type_checker::type_checker s_checker_reg(checker, 100);
}

symlink::symlink(const string &path)
  : object(path)
{
  _content_type = CONTENT_TYPE;
  _stat.st_mode |= S_IFLNK;
}

symlink::~symlink()
{
}
