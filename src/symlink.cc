#include "symlink.h"

using namespace boost;
using namespace std;

using namespace s3;

namespace
{
  const string CONTENT_TYPE = "text/symlink";
}

const string & symlink::get_default_content_type()
{
  return CONTENT_TYPE;
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
