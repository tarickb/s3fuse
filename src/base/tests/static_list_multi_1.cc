#include "base/tests/static_list_multi.h"

using std::string;

using s3::base::tests::void_fn_list;
using s3::base::tests::int_fn_list;

string void_fn_1()
{
  return "void fn 1";
}

string int_fn_1(int a)
{
  return "int fn 1";
}

void_fn_list::entry vf1(void_fn_1, 1);
int_fn_list::entry if1(int_fn_1, 1);
