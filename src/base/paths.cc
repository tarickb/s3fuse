#include <stdlib.h>

#include "base/paths.h"

using std::string;

using s3::base::paths;

string paths::transform(const string &path)
{
  if (!path.empty() && path[0] == '~')
    return string(getenv("HOME")) + (path.c_str() + 1);
  else
    return path;
}
