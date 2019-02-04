#ifndef S3_BASE_TESTS_STATIC_LIST_MULTI_H
#define S3_BASE_TESTS_STATIC_LIST_MULTI_H

#include <functional>
#include <string>

#include "base/static_list.h"

namespace s3
{
  namespace base
  {
    namespace tests
    {
      typedef static_list<std::function<std::string()>> void_fn_list;
      typedef static_list<std::function<std::string(int)>> int_fn_list;
    }
  }
}

#endif
