#include "encrypted_file.h"
#include "logger.h"
#include "request.h"

using namespace boost;
using namespace std;

using namespace s3;

namespace
{
  const string CONTENT_TYPE = "binary/encrypted-s3fuse-file";

  object * checker(const string &path, const request::ptr &req)
  {
    S3_LOG(LOG_DEBUG, "encrypted_file::checker", "testing [%s]\n", path.c_str());

    if (req->get_response_header("Content-Type") != CONTENT_TYPE)
      return NULL;

    return new encrypted_file(path);
  }

  object::type_checker::type_checker s_checker_reg(checker, 100);
}

encrypted_file::encrypted_file(const string &path)
  : object(path)
{
  _content_type = CONTENT_TYPE;
}

encrypted_file::~encrypted_file()
{
}
