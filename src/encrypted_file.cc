#include "encrypted_file.h"

using namespace boost;
using namespace std;

using namespace s3;

namespace
{
  const string CONTENT_TYPE = "binary/encrypted-s3fuse-file";
}

const string & encrypted_file::get_default_content_type()
{
  return CONTENT_TYPE;
}

encrypted_file::encrypted_file(const string &path)
  : object(path)
{
  _content_type = CONTENT_TYPE;
}

encrypted_file::~encrypted_file()
{
}
