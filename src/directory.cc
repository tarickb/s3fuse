#include "directory.h"
#include "logger.h"
#include "request.h"
#include "service.h"

using namespace boost;
using namespace std;

using namespace s3;

namespace
{
  object * checker(const string &path, const request::ptr &req)
  {
    const string &url = req->get_url();

    S3_LOG(LOG_DEBUG, "directory::checker", "testing [%s]\n", path.c_str());

    if (url.empty() || url[url.size() - 1] != '/')
      return NULL;

    return new directory(path);
  }

  object::type_checker::type_checker s_checker_reg(checker, 10);
}

string directory::build_url(const string &path)
{
  return service::get_bucket_url() + "/" + util::url_encode(path) + "/";
}

directory::directory(const string &path)
  : object(path)
{
  _url = build_url(path);
  _stat.st_mode |= S_IFDIR;
}

directory::~directory()
{
}
