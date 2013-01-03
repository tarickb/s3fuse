#ifndef S3_BASE_TESTS_STATIC_LIST_MULTI_H
#define S3_BASE_TESTS_STATIC_LIST_MULTI_H

#include <string>
#include <boost/function.hpp>

#include "base/static_list.h"

namespace s3
{
  namespace base
  {
    namespace tests
    {
      typedef static_list<boost::function0<std::string> > void_fn_list;
      typedef static_list<boost::function1<std::string, int> > int_fn_list;
    }
  }
}

#endif
