#include "base/tests/static_list_multi.h"

namespace s3
{
  namespace base
  {
    namespace tests
    {

std::string void_fn_2()
{
  return "void fn 2";
}

std::string int_fn_2(int a)
{
  return "int fn 2";
}

void_fn_list::entry vf2(void_fn_2, 100);
int_fn_list::entry if2(int_fn_2, 10);

    }
  }
}
