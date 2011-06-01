#include "logger.h"

using namespace s3;

int logger::s_max_level = 0;

void logger::init(int max_level)
{
  s_max_level = max_level;

  openlog("s3fuse", 0, 0);
}
