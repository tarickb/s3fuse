#include "directory.h"
#include "service.h"

using namespace boost;
using namespace std;

using namespace s3;

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
