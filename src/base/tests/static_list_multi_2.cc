#include "base/tests/static_list_multi.h"

using std::string;

using s3::base::tests::void_fn_list;
using s3::base::tests::int_fn_list;

string void_fn_2()
{
  return "void fn 2";
}

string int_fn_2(int a)
{
  return "int fn 2";
}

void_fn_list::entry vf2(void_fn_2, 100);
int_fn_list::entry if2(int_fn_2, 10);
