#include <stdlib.h>

#include <stdexcept>

#include "temp_file.h"

using namespace std;

using namespace s3;

temp_file::temp_file()
{
  char name[] = "/tmp/s3fuse.local-XXXXXX";

  _fd = mkstemp(name);

  if (_fd == -1)
    throw runtime_error("unable to create temporary file.");

  unlink(name);
}

temp_file::~temp_file()
{
  close(_fd);
}
