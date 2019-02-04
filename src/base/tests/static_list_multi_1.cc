#include "base/tests/static_list_multi.h"

namespace s3 {
  namespace base {
    namespace tests {

std::string void_fn_1()
{
  return "void fn 1";
}

std::string int_fn_1(int a)
{
  return "int fn 1";
}

void_fn_list::entry vf1(void_fn_1, 1);
int_fn_list::entry if1(int_fn_1, 1);

}  // namespace tests
}  // namespace base
}  // namespace s3
