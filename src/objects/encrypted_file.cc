#include "base/logger.h"
#include "base/request.h"
#include "objects/encrypted_file.h"

using std::string;

using s3::base::request;
using s3::objects::encrypted_file;
using s3::objects::object;

namespace
{
  const string CONTENT_TYPE = "binary/encrypted-s3fuse-file_0100"; // version 1.0

  object * checker(const string &path, const request::ptr &req)
  {
    if (req->get_response_header("Content-Type") != CONTENT_TYPE)
      return NULL;

    return new encrypted_file(path);
  }

  object::type_checker::type_checker s_checker_reg(checker, 100);
}

encrypted_file::encrypted_file(const string &path)
  : file(path)
{
  set_content_type(CONTENT_TYPE);
}

encrypted_file::~encrypted_file()
{
}
